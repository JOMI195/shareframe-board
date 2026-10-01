import type { ConnectionModeState } from '@/store/connectionMode/connectionMode.Slice';

export type ConnectionModePayload = Omit<ConnectionModeState, 'loaded'>;

export const makeConnectionMode = (over: Partial<ConnectionModePayload> = {}): ConnectionModePayload => ({
  mode: 'connected',
  ssid: 'HomeNet',
  internet: true,
  ap_ssid: 'shareframe-board',
  ap_password: '',
  ...over,
});
