// The board's mDNS name equals its hotspot name (set-hostname on the board).
export const boardAddress = (apSsid: string) => `http://${apSsid || 'shareframe-board'}.local`;
