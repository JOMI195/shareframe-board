import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { makeUpdateStatus } from '../../fixtures';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

export const updatesHandlers = [
  http.get(apiUrl(api.getLatestReleaseUrl()), () => ok(null)),
  http.post(apiUrl(api.getPerformUpdateUrl()), () => ok()),
  http.get(apiUrl(api.getUpdateStatusUrl()), () => ok(makeUpdateStatus())),
  http.get(apiUrl(api.getUpdateHistoryUrl()), () => ok({ history: [] })),
];
