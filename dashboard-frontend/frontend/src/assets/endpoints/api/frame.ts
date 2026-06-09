const apiBaseUrl = () => "/api";
const frameBaseUrl = () => `${apiBaseUrl()}/frame`;
const systemBaseUrl = () => `${apiBaseUrl()}/system`;

// Connection (WiFi)
export const getConnectionBaseUrl = () => `${apiBaseUrl()}/connection`;
export const getConnectionStatusUrl = () => `${getConnectionBaseUrl()}/status`;
export const getConnectionSavedNetworksUrl = () => `${getConnectionBaseUrl()}/saved-networks`;
export const getConnectionConnectUrl = () => `${getConnectionBaseUrl()}/connect`;
export const getConnectionForgetUrl = () => `${getConnectionBaseUrl()}/forget`;

// Frame / slideshow
export const getSlideshowUrl = () => `${frameBaseUrl()}/slideshow`;
export const getSlideshowStatusUrl = () => `${frameBaseUrl()}/slideshow/status`;
export const getSlideshowIntervalUrl = () => `${frameBaseUrl()}/slideshow/interval`;
export const getSkipSlideshowImageUrl = () => `${frameBaseUrl()}/slideshow/skip`;
export const getClearDisplayUrl = () => `${frameBaseUrl()}/clear`;

// Service management
const servicesBaseUrl = () => `${apiBaseUrl()}/services`;
export const getServicesUrl = () => servicesBaseUrl();
export const getServiceRestartUrl = () => `${servicesBaseUrl()}/restart`;

// System
export const getSystemInfoUrl = () => `${systemBaseUrl()}/info`;
export const getSystemHealthUrl = () => `${systemBaseUrl()}/health`;
export const getRestartPiUrl = () => `${systemBaseUrl()}/restart`;
export const getShutdownPiUrl = () => `${systemBaseUrl()}/shutdown`;
export const getFrameLogsUrl = () => `${systemBaseUrl()}/logs`;
export const getLatestReleaseUrl = () => `${systemBaseUrl()}/updates/latest`;
// NOTE: no backend route yet (RAUC install flow is out of scope). The
// "perform update" button is disabled, so this is never actually called.
export const getPerformUpdateUrl = () => `${systemBaseUrl()}/updates/perform-update`;
