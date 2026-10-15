import { readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { expect, type APIRequestContext, type TestInfo } from '@playwright/test';
import { PNG } from 'pngjs';

const PANEL_DIR = process.env.PANEL_DIR ?? '/panel';
const STATE_DIR = process.env.STATE_DIR ?? '/state';
const FAKE_SERVER = process.env.FAKE_SERVER_URL ?? 'http://fake-server:9000';
export const FIXTURES = path.resolve(import.meta.dirname, '../fixtures');
export const DASHBOARD_PASSWORD = 'e2e-password';
export const INSTALLED_VERSION = '1.2.0';

// ---- virtual panel

export type Panel = { state: 'image' | 'cleared'; source: string; seq: number };

export async function readPanel(): Promise<Panel | null> {
  try {
    return JSON.parse(await readFile(path.join(PANEL_DIR, 'panel.json'), 'utf8'));
  } catch {
    return null;
  }
}

/** Polls the virtual panel until it satisfies `predicate`; returns that state. */
export async function waitForPanel(predicate: (p: Panel) => boolean, message: string, timeout = 20_000) {
  const deadline = Date.now() + timeout;
  let last: Panel | null = null;
  while (Date.now() < deadline) {
    last = await readPanel();
    if (last && predicate(last)) return last;
    await new Promise((r) => setTimeout(r, 100));
  }
  throw new Error(`Timed out waiting for: ${message}. Last panel: ${JSON.stringify(last)}`);
}

export const showsImage = (id: number) => (p: Panel) => p.state === 'image' && p.source.endsWith(`_${id}.jpg`);
export const showsDefault = (p: Panel) => p.state === 'image' && p.source.includes('/assets/image/default/');
export const isCleared = (p: Panel) => p.state === 'cleared';

/** The 1-bit frame the e-paper would get, decoded from the PBM the mock display writes. */
export class Frame {
  private constructor(readonly width: number, readonly height: number, private readonly bits: Buffer) {}

  static async read() {
    const pbm = await readFile(path.join(PANEL_DIR, 'panel.pbm'));
    const header = pbm.subarray(0, 16).toString('latin1').match(/^P4\n(\d+) (\d+)\n/);
    if (!header) throw new Error('panel.pbm has no P4 header');
    return new Frame(Number(header[1]), Number(header[2]), pbm.subarray(header[0].length));
  }

  isBlack(x: number, y: number) {
    const i = y * this.width + x;
    return ((this.bits[i >> 3] >> (7 - (i & 7))) & 1) === 1;
  }

  /** Share of black pixels in the rectangle, 0..1. */
  blackRatio(x0: number, y0: number, x1: number, y1: number) {
    let black = 0;
    for (let y = y0; y < y1; y++) for (let x = x0; x < x1; x++) if (this.isBlack(x, y)) black++;
    return black / ((x1 - x0) * (y1 - y0));
  }

  png() {
    const png = new PNG({ width: this.width, height: this.height });
    for (let y = 0; y < this.height; y++)
      for (let x = 0; x < this.width; x++) {
        const v = this.isBlack(x, y) ? 0 : 255;
        png.data.set([v, v, v, 255], (y * this.width + x) * 4);
      }
    return PNG.sync.write(png);
  }
}

export async function attachPanel(testInfo: TestInfo, name: string) {
  await testInfo.attach(name, { body: (await Frame.read()).png(), contentType: 'image/png' });
}

// ---- board side effects (shims) and network state

export async function shimCalls(): Promise<string[]> {
  const log = await readFile(path.join(STATE_DIR, 'shim-calls.log'), 'utf8').catch(() => '');
  return log.split('\n').filter(Boolean);
}

export async function expectShimCall(line: string) {
  await expect.poll(shimCalls, { message: `board ran "${line}"` }).toContain(line);
}

export async function resetBoardState() {
  for (const file of ['wifi-mode.json', 'wifi-mode', 'wifi-networks'])
    await writeFile(path.join(STATE_DIR, file), await readFile(path.join(STATE_DIR, 'defaults', file)));
  await writeFile(path.join(STATE_DIR, 'shim-calls.log'), '');
}

/** Switches what the board's Wi-Fi daemon reports, e.g. the hotspot fallback. */
export async function setWifiMode(mode: 'ap' | 'connected', extra: Record<string, unknown> = {}) {
  const json = { mode, ssid: mode === 'ap' ? '' : 'E2E-WiFi', internet: mode !== 'ap', ap_ssid: 'shareframe-e2e', ap_password: 'hotspot123', ...extra };
  await writeFile(path.join(STATE_DIR, 'wifi-mode.json'), JSON.stringify(json));
  await writeFile(path.join(STATE_DIR, 'wifi-mode'), `${mode}\n`);
}

// ---- fake Shareframe server

type ServerState = {
  connectedFrames: number;
  tokenRequests: number;
  heartbeats: Record<string, unknown>[];
  frameMessages: Record<string, unknown>[];
  bundleDownloads: { name: string; authorized: boolean }[];
};

export class FakeServer {
  constructor(private readonly request: APIRequestContext) {}

  async state(): Promise<ServerState> {
    return (await this.request.get(`${FAKE_SERVER}/__test/state`)).json();
  }

  async waitForFrame() {
    await expect.poll(async () => (await this.state()).connectedFrames, { message: 'frame connected to fake server' }).toBeGreaterThan(0);
  }

  async reset() {
    await this.waitForFrame();
    await this.request.post(`${FAKE_SERVER}/__test/reset`);
  }

  async send(message: Record<string, unknown>) {
    await this.waitForFrame();
    const res = await this.request.post(`${FAKE_SERVER}/__test/send`, { data: message });
    expect((await res.json()).delivered).toBeGreaterThan(0);
  }

  /** Sends text as-is, for frames a real server should never produce. */
  async sendRaw(text: string) {
    await this.waitForFrame();
    const res = await this.request.post(`${FAKE_SERVER}/__test/send-raw`, { data: text, headers: { 'Content-Type': 'text/plain' } });
    expect((await res.json()).delivered).toBeGreaterThan(0);
  }

  /** Pushes a fixture image as a new picture and returns the id the server gave it. */
  async sendPicture(fixture: string, opts: { expiresInSecs?: number; id?: number; data?: string } = {}): Promise<number> {
    await this.waitForFrame();
    const data = opts.data ?? (await readFile(path.join(FIXTURES, fixture))).toString('base64');
    const res = await this.request.post(`${FAKE_SERVER}/__test/picture`, { data: { ...opts, data } });
    const body = await res.json();
    expect(body.delivered).toBeGreaterThan(0);
    return body.id;
  }

  async clearImages(ids: number[]) {
    await this.send({ type: 'clear_specific_sent_images', sent_image_ids: ids });
  }

  async offerRelease(release: { version: string; release_notes?: string; criticality?: string; corrupt?: boolean }) {
    await this.request.post(`${FAKE_SERVER}/__test/release`, {
      data: { release_notes: 'E2E release', release_date: '2026-10-01T10:00:00Z', criticality: 'normal', ...release },
    });
  }

  /** Ends the frame's socket with a close code, or drops it without one. */
  async disconnect(code?: number) {
    await this.request.post(`${FAKE_SERVER}/__test/disconnect`, { data: code ? { code } : {} });
  }
}

// ---- dashboard REST API

/** Dashboard REST API through the frontend's nginx, signed in with the device password. */
export class BoardApi {
  constructor(private readonly request: APIRequestContext) {}

  async login(password = DASHBOARD_PASSWORD) {
    const res = await this.request.post('/api/auth/login', { data: { password } });
    expect(res.ok(), await res.text()).toBe(true);
  }

  async get(url: string) {
    const res = await this.request.get(url);
    expect(res.ok(), `${url}: ${await res.text()}`).toBe(true);
    return (await res.json()).data;
  }

  postRaw(url: string, data: Record<string, unknown> = {}) {
    return this.request.post(url, { data });
  }

  async post(url: string, data: Record<string, unknown> = {}) {
    const res = await this.postRaw(url, data);
    expect(res.ok(), `${url}: ${await res.text()}`).toBe(true);
    return (await res.json()).data;
  }

  slideshowStatus() {
    return this.get('/api/frame/slideshow/status');
  }

  async services(): Promise<{ id: string; running: boolean; pid: number | null }[]> {
    return (await this.get('/api/services')).services;
  }

  /** Restarts a service through the dashboard and waits until it answers again under a new pid. */
  async restartService(id: string) {
    const before = (await this.services()).find((s) => s.id === id)?.pid;
    await this.post('/api/services/restart', { service: id });
    await expect
      .poll(async () => {
        const svc = (await this.services()).find((s) => s.id === id);
        return svc?.running && svc.pid !== before;
      }, { message: `${id} restarted`, timeout: 20_000 })
      .toBe(true);
  }
}
