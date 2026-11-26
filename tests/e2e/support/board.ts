import { execFile } from 'node:child_process';
import { cp, readdir, readFile, rm, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { expect, type APIRequestContext, type TestInfo } from '@playwright/test';
import { PNG } from 'pngjs';

const PANEL_DIR = process.env.PANEL_DIR ?? '/panel';
const STATE_DIR = process.env.STATE_DIR ?? '/state';
const FAKE_SERVER = process.env.FAKE_SERVER_URL ?? 'http://fake-server:9000';
// ssh arguments for a board that is not a local container (the simulator).
const BOARD_SSH = process.env.BOARD_SSH?.split(' ').filter(Boolean);
export const FIXTURES = path.resolve(import.meta.dirname, '../fixtures');
export const DASHBOARD_PASSWORD = 'e2e-password';
export const INSTALLED_VERSION = '1.2.0';

/** Runs a shell command on the simulated board over ssh. */
export function onBoard(command: string): Promise<Buffer> {
  if (!BOARD_SSH) throw new Error('BOARD_SSH is not set');
  return new Promise((resolve, reject) =>
    execFile('ssh', [...BOARD_SSH, command], { encoding: 'buffer', maxBuffer: 4 << 20 }, (err, stdout, stderr) =>
      err ? reject(new Error(`${command}: ${stderr.toString() || err.message}`)) : resolve(stdout),
    ),
  );
}

const panelFile = (name: string) =>
  BOARD_SSH ? onBoard(`cat ${PANEL_DIR}/${name}`) : readFile(path.join(PANEL_DIR, name));

// ---- virtual panel

export type Panel = { state: 'image' | 'cleared'; source: string; seq: number; text?: string[] };

/** The 6-digit code a screen shows ("482 913"), read like the owner reads it off the frame. */
export const codeOn = (p: Panel): string => {
  const match = (p.text ?? []).join(' ').match(/(\d{3}) (\d{3})/);
  if (!match) throw new Error(`No code on the panel: ${JSON.stringify(p.text)}`);
  return match[1] + match[2];
};

export async function readPanel(): Promise<Panel | null> {
  try {
    return JSON.parse((await panelFile('panel.json')).toString('utf8'));
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
    await new Promise((r) => setTimeout(r, BOARD_SSH ? 1000 : 100));
  }
  throw new Error(`Timed out waiting for: ${message}. Last panel: ${JSON.stringify(last)}`);
}

export const showsImage = (id: number) => (p: Panel) => p.state === 'image' && p.source.endsWith(`_${id}.jpg`);
// The bundled set, or the owner's copy of it on /data.
export const showsDefault = (p: Panel) =>
  p.state === 'image' && (p.source.includes('/assets/image/default/') || p.source.includes('/default-images/'));
export const isCleared = (p: Panel) => p.state === 'cleared';
export const showsApSetup = (p: Panel) => p.state === 'image' && p.source === 'ap-setup';
export const showsRegistration = (p: Panel) => p.state === 'image' && p.source === 'registration';
export const showsStatus = (screen: string) => (p: Panel) => p.state === 'image' && p.source === `status:${screen}`;

/** The 1-bit frame the e-paper would get, decoded from the PBM the mock display writes. */
export class Frame {
  private constructor(readonly width: number, readonly height: number, private readonly bits: Buffer) {}

  static async read() {
    const pbm = await panelFile('panel.pbm');
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

// ---- board side effects (the image's scripts and the shims) and network state

export async function shimCalls(): Promise<string[]> {
  const log = await readFile(path.join(STATE_DIR, 'shim-calls.log'), 'utf8').catch(() => '');
  return log.split('\n').filter(Boolean);
}

export async function expectShimCall(line: string) {
  await expect.poll(shimCalls, { message: `board ran "${line}"` }).toContain(line);
}

/** Where the board's dashboard asks the Wi-Fi daemon to leave the hotspot. */
export const retryFlag = path.join(STATE_DIR, 'wifi-retry-requested');

export async function resetBoardState() {
  await cp(path.join(STATE_DIR, 'defaults'), STATE_DIR, { recursive: true });
  const own = (f: string) => path.join(STATE_DIR, 'shareframe', f);
  for (const stale of [retryFlag, path.join(STATE_DIR, 'factory-reset'), own('remote-reboot-at'), own('safe-mode-remote'), own('update-history.jsonl')])
    await rm(stale, { force: true });
  await rm(own('default-images'), { recursive: true, force: true });
  const rauc = path.join(STATE_DIR, 'rauc');
  for (const f of await readdir(rauc).catch(() => [])) await rm(path.join(rauc, f), { recursive: true, force: true });
  await writeFile(path.join(STATE_DIR, 'shim-calls.log'), '');
}

export const wifiConf = () => readFile(path.join(STATE_DIR, 'wifi', 'wpa_supplicant.conf'), 'utf8');

/** SSIDs in the board's wpa_supplicant.conf, "quoted" or hex as wpa_supplicant saves them. */
export async function savedWifiNetworks(): Promise<string[]> {
  return [...(await wifiConf()).matchAll(/^\s*ssid=(.*)$/gm)].map(([, v]) =>
    v.startsWith('"') ? v.slice(1, -1) : Buffer.from(v, 'hex').toString('utf8'));
}

/** The network is saved on the board, with its password hashed, never in plain text. */
export async function expectSavedNetwork(ssid: string, password: string) {
  await expect.poll(savedWifiNetworks, { message: `${ssid} saved on the board` }).toContain(ssid);
  expect(await wifiConf(), 'plain-text password in wpa_supplicant.conf').not.toContain(password);
}

/** The hotspot credentials as the board's ap.conf holds them (shell-sourced on the board). */
export async function apConf(): Promise<string> {
  return readFile(path.join(STATE_DIR, 'wifi', 'ap.conf'), 'utf8');
}

/** The slot an installed update waits in (/data/rauc/pending), or '' when none does. */
export async function pendingSlot(): Promise<string> {
  return (await readFile(path.join(STATE_DIR, 'rauc', 'pending'), 'utf8').catch(() => '')).trim();
}

/** shareframe-factory-reset left its mark for the next boot's wipe. */
export async function factoryResetScheduled(): Promise<boolean> {
  return readFile(path.join(STATE_DIR, 'factory-reset')).then(() => true, () => false);
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
  diagnostics: { trigger: string; commandId: string | null; text: string | null }[];
};

type CommandResult = { type: 'command_result'; id: number; status: string; reason: string };

export class FakeServer {
  constructor(private readonly request: APIRequestContext) {}

  async state(): Promise<ServerState> {
    return (await this.request.get(`${FAKE_SERVER}/__test/state`)).json();
  }

  /** Sends an admin's remote command; returns its id. */
  async sendCommand(command: string, arg = '', opts: { expiresInSecs?: number } = {}): Promise<number> {
    const res = await this.request.post(`${FAKE_SERVER}/__test/command`, { data: { command, arg, ...opts } });
    return (await res.json()).id;
  }

  /** The statuses the frame reported for a command, in order (accepted, done, rejected, …). */
  async commandResults(id: number): Promise<CommandResult[]> {
    return (await this.state()).frameMessages.filter(
      (m): m is CommandResult => m.type === 'command_result' && m.id === id);
  }

  async expectCommandResult(id: number, status: string): Promise<CommandResult> {
    await expect
      .poll(async () => (await this.commandResults(id)).map((r) => r.status), { message: `command #${id} ${status}`, timeout: 30_000 })
      .toContain(status);
    return (await this.commandResults(id)).find((r) => r.status === status)!;
  }

  async diagnostics() {
    return (await this.state()).diagnostics;
  }

  async waitForFrame() {
    await expect.poll(async () => (await this.state()).connectedFrames, { message: 'frame connected to fake server' }).toBeGreaterThan(0);
  }

  async reset() {
    await this.unregistered(false);
    await this.outage(false);
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
  async sendPicture(fixture: string, opts: { expiresInSecs?: number; id?: number; data?: string; sender?: string } = {}): Promise<number> {
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

  async offerRelease(release: { version: string; release_notes?: string; criticality?: string; corrupt?: boolean; bundleFile?: string }) {
    await this.request.post(`${FAKE_SERVER}/__test/release`, {
      data: { release_notes: 'E2E release', release_date: '2026-10-01T10:00:00Z', criticality: 'normal', ...release },
    });
  }

  /** While on, the server refuses the frame because nobody owns it. */
  async unregistered(on: boolean) {
    await this.request.post(`${FAKE_SERVER}/__test/unregistered`, { data: { on } });
  }

  /** While on, the frame is dropped and every reconnect is turned away. */
  async outage(on: boolean) {
    await this.request.post(`${FAKE_SERVER}/__test/outage`, { data: { on } });
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

  /** Skips the current image, retrying while the board rejects skips because the panel is refreshing. */
  async skip() {
    await expect
      .poll(async () => (await this.postRaw('/api/frame/slideshow/skip')).status(), { message: 'skip accepted' })
      .toBe(200);
  }

  /** Adds a default image straight through the API, the way the dashboard sends one. */
  async uploadDefaultImage(fixture: string): Promise<string> {
    const before = await this.defaultImageNames();
    const res = await this.request.post('/api/frame/default-images', {
      data: await readFile(path.join(FIXTURES, fixture)),
      headers: { 'Content-Type': 'image/jpeg' },
    });
    expect(res.ok(), await res.text()).toBe(true);
    return (await this.defaultImageNames()).find((name) => !before.includes(name))!;
  }

  async defaultImageNames(): Promise<string[]> {
    return (await this.get('/api/frame/default-images')).images.map((image: { name: string }) => image.name);
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
    // A new pid comes before the service binds its IPC socket.
    const probe = SERVICE_PROBES[id];
    if (probe)
      await expect
        .poll(async () => (await this.request.get(probe)).ok(), { message: `${id} answers again`, timeout: 20_000 })
        .toBe(true);
  }
}

const SERVICE_PROBES: Record<string, string> = {
  display: '/api/frame/slideshow/status',
  update: '/api/system/updates/status',
};
