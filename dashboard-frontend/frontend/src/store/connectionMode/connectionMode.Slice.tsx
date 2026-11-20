import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getConnectionModeUrl } from '@/assets/endpoints/api/frame';

// Network mode published by the board's wifi-mode-manager daemon plus the
// websocket's server link, exposed (publicly) at GET /api/connection/mode.
// Drives the status banner, the header indicator and the hotspot connect flow.
export type WifiMode = 'connecting' | 'connected' | 'ap';

export interface ConnectionModeState {
    mode: WifiMode;
    ssid: string;
    internet: boolean;
    ap_ssid: string;
    ap_password: string;
    // The board's mDNS name (shareframe-board-XXXX), how to find it on the home network.
    hostname: string;
    // null while the board can't tell; server_last_contact_at is unix seconds, 0 = never connected.
    server_connected: boolean | null;
    // false once the server refused the frame because nobody owns it; null while the board can't tell.
    registered: boolean | null;
    // Why the board keeps the display down (remote, display-crash, boot-loop); empty while it runs.
    safe_mode: string;
    server_offline_secs: number;
    server_last_contact_at: number;
    // How long ago the last try of the saved networks failed and the hotspot came back; null = none.
    last_failed_secs_ago: number | null;
    // The same moment on the browser's clock (ms), to compare with actions taken here.
    last_failed_at: number | null;
    loaded: boolean;
}

const initialState: ConnectionModeState = {
    mode: 'connecting',
    ssid: '',
    internet: false,
    ap_ssid: '',
    ap_password: '',
    hostname: '',
    server_connected: null,
    registered: null,
    safe_mode: '',
    server_offline_secs: 0,
    server_last_contact_at: 0,
    last_failed_secs_ago: null,
    last_failed_at: null,
    loaded: false,
};

export const fetchConnectionMode = createAsyncThunk(
    'connectionMode/fetch',
    async (_, { rejectWithValue }) => {
        try {
            // Pessimistic timeout: tolerate slow/bad links during connecting.
            const res = await fetchWithTimeout(getConnectionModeUrl(), {}, 60000);
            const payload = await res.json();
            if (payload?.success) {
                return payload.data as Partial<ConnectionModeState>;
            }
            return rejectWithValue('Unexpected response');
        } catch (error) {
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const connectionModeSlice = createSlice({
    name: 'connectionMode',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder.addCase(fetchConnectionMode.fulfilled, (state, action) => {
            const d = action.payload ?? {};
            state.mode = (d.mode as WifiMode) ?? 'connecting';
            state.ssid = d.ssid ?? '';
            state.internet = !!d.internet;
            state.ap_ssid = d.ap_ssid ?? '';
            state.ap_password = d.ap_password ?? '';
            state.hostname = d.hostname ?? '';
            state.server_connected = d.server_connected ?? null;
            state.registered = d.registered ?? null;
            state.safe_mode = d.safe_mode ?? '';
            state.server_offline_secs = d.server_offline_secs ?? 0;
            state.server_last_contact_at = d.server_last_contact_at ?? 0;
            state.last_failed_secs_ago = d.last_failed_secs_ago ?? null;
            state.last_failed_at = state.last_failed_secs_ago === null
                ? null : Date.now() - state.last_failed_secs_ago * 1000;
            state.loaded = true;
        });
    },
});

export const selectConnectionMode = (state: RootState) => state.connectionMode;

// A reconnect normally takes seconds; only a longer gap counts as the server being down.
const SERVER_GRACE_SECS = 120;

export const isServerDown = ({ server_connected, server_last_contact_at, server_offline_secs }: ConnectionModeState) =>
    server_connected === false && (server_last_contact_at === 0 || server_offline_secs > SERVER_GRACE_SECS);

export default connectionModeSlice.reducer;
