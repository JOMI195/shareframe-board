const apiBaseUrl = () => "/api";
const frameBaseUrl = () => `${apiBaseUrl()}/frame`;
//const connectionBaseUrl = () => `${apiBaseUrl()}/connection`;


// Auth


// Connection
export const getConnectionBaseUrl = () => `${apiBaseUrl()}/connection`;

export const getConnectionForgetUrl = () => `${getConnectionBaseUrl()}/forget`;
export const getConnectionRenameUrl = () => `${getConnectionBaseUrl()}/rename`;

// Slideshow
export const getSlideshowUrl = () => `${frameBaseUrl()}/slideshow`;

export const getClearDisplayUrl = () => `${frameBaseUrl()}/clear`;

export const getSlideshowIsActiveUrl = () => `${frameBaseUrl()}/slideshow/is-active`;

export const getSkipSlideshowImageUrl = () => `${frameBaseUrl()}/slideshow/skip-slideshow-image`;

export const getDisplayImagesLoopIntervalUrl = () => `${frameBaseUrl()}/slideshow/display-images-loop-interval`;

// Updates
export const getLatestReleaseUrl = () => `${frameBaseUrl()}/updates/latest`;

export const getPerformUpdateUrl = () => `${frameBaseUrl()}/updates/perform-update`;

// PiPower
export const getRestartPiUrl = () => `${apiBaseUrl()}/pi/restart`;

export const getShutdownPiUrl = () => `${apiBaseUrl()}/pi/shutdown`;

// Logs
export const getFrameLogsUrl = () => `${apiBaseUrl()}/logs`;
