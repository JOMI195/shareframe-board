import type { FrameInfo } from '@/store/frameInfo/frameInfo.Slice';

export const makeFrameInfo = (over: Partial<FrameInfo> = {}): FrameInfo => ({
  serial_number: 'AAAA-BBBB-CCCC-DDDD',
  version: '1.2.0',
  hostname: 'shareframe-board',
  ip_wlan0: '192.168.0.42',
  uptime_seconds: 3600,
  cpu_temp_celsius: 45.2,
  wlan_ssid: 'HomeNet',
  wlan_signal_dbm: -55,
  ...over,
});
