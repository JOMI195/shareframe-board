import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { makeConnectionMode } from '../../fixtures';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

export const connectionHandlers = [
  http.get(apiUrl(api.getConnectionModeUrl()), () => ok(makeConnectionMode())),
  http.get(apiUrl(api.getConnectionStatusUrl()), () => ok({ connection_name: 'HomeNet' })),
  http.get(apiUrl(api.getConnectionSavedNetworksUrl()), () => ok({ networks: ['HomeNet'] })),
  http.post(apiUrl(api.getConnectionConnectUrl()), () => ok()),
  http.post(apiUrl(api.getConnectionForgetUrl()), () => ok()),
  http.post(apiUrl(api.getApPasswordUrl()), () => ok()),
];
