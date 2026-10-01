import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { healthyServices } from '../../fixtures';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

export const servicesHandlers = [
  http.get(apiUrl(api.getServicesUrl()), () => ok({ services: healthyServices() })),
  http.post(apiUrl(api.getServiceRestartUrl()), () => ok(null, 'Dienst wird neu gestartet')),
];
