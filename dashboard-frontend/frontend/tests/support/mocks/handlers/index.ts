import { authHandlers } from './auth';
import { connectionHandlers } from './connection';
import { frameHandlers, resetBoard, setBoard } from './frame';
import { servicesHandlers } from './services';
import { systemHandlers } from './system';
import { updatesHandlers } from './updates';

// The default set answers every call the app can make with a healthy, signed-in,
// online board. Tests layer data or failures on top with server.use(...scenario).
export const handlers = [
  ...authHandlers,
  ...connectionHandlers,
  ...frameHandlers,
  ...systemHandlers,
  ...servicesHandlers,
  ...updatesHandlers,
];

export {
  authHandlers,
  connectionHandlers,
  frameHandlers,
  resetBoard,
  setBoard,
  servicesHandlers,
  systemHandlers,
  updatesHandlers,
};
