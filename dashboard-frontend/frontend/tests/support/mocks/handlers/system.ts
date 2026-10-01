import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { makeFrameInfo } from '../../fixtures';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

export const systemHandlers = [
  http.get(apiUrl(api.getSystemHealthUrl()), () => ok({ running: true })),
  http.get(apiUrl(api.getSystemInfoUrl()), () => ok(makeFrameInfo())),
  http.post(apiUrl(api.getRestartPiUrl()), () => ok()),
  http.post(apiUrl(api.getShutdownPiUrl()), () => ok()),
  http.get(apiUrl(api.getFrameLogsUrl()), ({ request }) => {
    const service = new URL(request.url).searchParams.get('service_name') ?? '';
    return ok({ service_name: service, lines: 0, logs: '' });
  }),
];
