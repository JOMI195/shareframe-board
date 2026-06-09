import { combineReducers } from "redux";
import snackbarsReducer from "./snackbars/snackbars.Slice";
import slideshowOperationReducer from "./slideshowOperation/slideshowOperation.Slice";
import slideshowStatusReducer from "./slideshowStatus/slideshowStatus.Slice";
import timersReducer from "./timers/timers.Slice";
import networkReducer from "./network/network.Slice";
import authReducer from "./auth/auth.Slice";
import frameInfoReducer from "./frameInfo/frameInfo.Slice";
import displayStatsReducer from "./displayStats/displayStats.Slice";
import updatesReducer from "./updates/updates.Slice";
import loadingWallReducer from "./loadingWall/loadingWall.Slice";
import piPowerReducer from "./piPower/piPower.Slice";
import frameLogsReducer from "./frameLogs/frameLogs.Slice";
import navigationReducer from "./navigation/navigation.Slice";
import dialogsReducer from "./dialogs/dialogs.Slice";
import servicesReducer from "./services/services.Slice";

const rootReducer = combineReducers({
  snackbars: snackbarsReducer,
  slideshowOperation: slideshowOperationReducer,
  slideshowStatus: slideshowStatusReducer,
  timers: timersReducer,
  network: networkReducer,
  auth: authReducer,
  frameInfo: frameInfoReducer,
  displayStats: displayStatsReducer,
  updates: updatesReducer,
  loadingWall: loadingWallReducer,
  piPower: piPowerReducer,
  frameLogs: frameLogsReducer,
  navigation: navigationReducer,
  dialogs: dialogsReducer,
  services: servicesReducer,
});

export default rootReducer;