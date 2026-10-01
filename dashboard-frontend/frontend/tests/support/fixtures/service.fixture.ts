import { SERVICE_LABELS, type ServiceStatus } from '@/types';

export const makeService = (id: string, over: Partial<ServiceStatus> = {}): ServiceStatus => ({
  id,
  label: SERVICE_LABELS[id] ?? id,
  running: true,
  status: 'up',
  uptime_seconds: 7200,
  pid: 100,
  ...over,
});

export const healthyServices = (): ServiceStatus[] =>
  ['display', 'websocket', 'dashboard', 'heartbeat', 'update'].map((id, i) => makeService(id, { pid: 100 + i }));
