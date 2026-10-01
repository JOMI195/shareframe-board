import { http, HttpResponse, type RequestHandler } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import type { DisplayStats } from '@/store/displayStats/displayStats.Slice';
import type { FrameInfo } from '@/store/frameInfo/frameInfo.Slice';
import type { UpdateHistoryEntry, UpdateStatus } from '@/store/updates/updates.Slice';
import type { ServiceStatus } from '@/types';
import {
  makeConnectionMode,
  makeSlideshowStatus,
  type ConnectionModePayload,
  type ReleaseFixture,
  type SlideshowStatusPayload,
} from '../fixtures';
import { apiUrl } from './apiUrl';
import { fail, ok } from './envelope';

type Method = 'get' | 'post';

export const signedOut = (): RequestHandler[] => [
  http.get(apiUrl(api.getAuthStatusUrl()), () => ok({ authenticated: false })),
];

export const withConnectionMode = (over: Partial<ConnectionModePayload>): RequestHandler[] => [
  http.get(apiUrl(api.getConnectionModeUrl()), () => ok(makeConnectionMode(over))),
];

/** Board hosts its own access point: no known WiFi, no internet. */
export const apMode = (over: Partial<ConnectionModePayload> = {}): RequestHandler[] =>
  withConnectionMode({ mode: 'ap', ssid: '', internet: false, ...over });

export const boardUnreachable = (): RequestHandler[] => [
  http.get(apiUrl(api.getSystemHealthUrl()), () => HttpResponse.error()),
];

export const withNetworks = (current: string, saved: string[]): RequestHandler[] => [
  http.get(apiUrl(api.getConnectionStatusUrl()), () => ok({ connection_name: current })),
  http.get(apiUrl(api.getConnectionSavedNetworksUrl()), () => ok({ networks: saved })),
];

export const withSlideshowStatus = (over: Partial<SlideshowStatusPayload>): RequestHandler[] => [
  http.get(apiUrl(api.getSlideshowStatusUrl()), () => ok(makeSlideshowStatus(over))),
];

export const withFrameInfo = (info: FrameInfo): RequestHandler[] => [
  http.get(apiUrl(api.getSystemInfoUrl()), () => ok(info)),
];

export const withDisplayStats = (stats: DisplayStats): RequestHandler[] => [
  http.get(apiUrl(api.getDisplayStatsUrl()), () => ok(stats)),
];

export const withServices = (services: ServiceStatus[]): RequestHandler[] => [
  http.get(apiUrl(api.getServicesUrl()), () => ok({ services })),
];

export const withRelease = (release: ReleaseFixture | null): RequestHandler[] => [
  http.get(apiUrl(api.getLatestReleaseUrl()), () => ok(release)),
];

export const withUpdateStatus = (status: UpdateStatus): RequestHandler[] => [
  http.get(apiUrl(api.getUpdateStatusUrl()), () => ok(status)),
];

export const withUpdateHistory = (history: UpdateHistoryEntry[]): RequestHandler[] => [
  http.get(apiUrl(api.getUpdateHistoryUrl()), () => ok({ history })),
];

export const withLogs = (logs: string[]): RequestHandler[] => [
  http.get(apiUrl(api.getFrameLogsUrl()), ({ request }) => {
    const service = new URL(request.url).searchParams.get('service_name') ?? '';
    return ok({ service_name: service, lines: logs.length, logs: logs.join('\n') });
  }),
];

/** Explicit `success: false` from the board, e.g. failsWith('post', api.getLoginUrl(), 401, 'Passwort ungültig'). */
export const failsWith = (method: Method, path: string, status = 500, message = 'Fehler'): RequestHandler[] => [
  http[method](apiUrl(path), () => fail(message, status)),
];

/** Network failure: the request never gets an answer. */
export const unreachable = (method: Method, path: string): RequestHandler[] => [
  http[method](apiUrl(path), () => HttpResponse.error()),
];

/** Records the requests hitting one endpoint so a test can assert on what it sent. */
export const spyOnRequests = (method: Method, path: string, respond: () => Response = () => ok()) => {
  const calls: Request[] = [];
  const handler = http[method](apiUrl(path), ({ request }) => {
    calls.push(request.clone());
    return respond();
  });

  return { handler, calls };
};
