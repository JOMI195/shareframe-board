// The board's own name on the home network; the hotspot name only as a fallback for older boards.
export const boardAddress = (hostname: string, apSsid = '') => `http://${hostname || apSsid || 'shareframe-board'}.local`;
