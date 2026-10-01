const apiBaseUrl = () => "/api";
const frameBaseUrl = () => `${apiBaseUrl()}/frame`;
const systemBaseUrl = () => `${apiBaseUrl()}/system`;
const authBaseUrl = () => `${apiBaseUrl()}/auth`;

// Auth
export const getAuthStatusUrl = () => `${authBaseUrl()}/status`;
export const getLoginUrl = () => `${authBaseUrl()}/login`;
export const getLogoutUrl = () => `${authBaseUrl()}/logout`;
export const getChangePasswordUrl = () => `${authBaseUrl()}/change-password`;

// Connection (WiFi)
export const getConnectionBaseUrl = () => `${apiBaseUrl()}/connection`;
export const getConnectionStatusUrl = () => `${getConnectionBaseUrl()}/status`;
export const getConnectionSavedNetworksUrl = () => `${getConnectionBaseUrl()}/saved-networks`;
export const getConnectionConnectUrl = () => `${getConnectionBaseUrl()}/connect`;
export const getConnectionForgetUrl = () => `${getConnectionBaseUrl()}/forget`;
export const getConnectionModeUrl = () => `${getConnectionBaseUrl()}/mode`;
export const getApPasswordUrl = () => `${getConnectionBaseUrl()}/ap-password`;

// Frame / slideshow
export const getSlideshowUrl = () => `${frameBaseUrl()}/slideshow`;
export const getSlideshowStatusUrl = () => `${frameBaseUrl()}/slideshow/status`;
export const getSlideshowIntervalUrl = () => `${frameBaseUrl()}/slideshow/interval`;
export const getSlideshowNightModeUrl = () => `${frameBaseUrl()}/slideshow/night-mode`;
export const getSkipSlideshowImageUrl = () => `${frameBaseUrl()}/slideshow/skip`;
export const getClearDisplayUrl = () => `${frameBaseUrl()}/clear`;
export const getDisplayStatsUrl = () => `${frameBaseUrl()}/display/stats`;

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
export const getPerformUpdateUrl = () => `${systemBaseUrl()}/updates/perform-update`;
export const getUpdateStatusUrl = () => `${systemBaseUrl()}/updates/status`;
export const getUpdateHistoryUrl = () => `${systemBaseUrl()}/updates/history`;
