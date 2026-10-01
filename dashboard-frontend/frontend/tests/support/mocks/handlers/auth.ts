import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

export const authHandlers = [
  http.get(apiUrl(api.getAuthStatusUrl()), () => ok({ authenticated: true })),
  http.post(apiUrl(api.getLoginUrl()), () => ok(null, 'Login erfolgreich')),
  http.post(apiUrl(api.getLogoutUrl()), () => ok()),
  http.post(apiUrl(api.getChangePasswordUrl()), () => ok()),
];
