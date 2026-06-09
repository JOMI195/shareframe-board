import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import { fetchWithTimeout } from '@/common/utils/fetch';

// Network mode published by the board's wifi-mode-manager daemon and exposed
// (publicly) at GET /api/connection/mode. Drives the status banner and the
// offline AP-setup flow.
export type WifiMode = 'connecting' | 'connected' | 'ap';

export interface ConnectionModeState {
    mode: WifiMode;
    ssid: string;
    internet: boolean;
    ap_ssid: string;
    ap_password: string;
    loaded: boolean;
}

const initialState: ConnectionModeState = {
    mode: 'connecting',
    ssid: '',
    internet: false,
    ap_ssid: '',
    ap_password: '',
    loaded: false,
};

export const fetchConnectionMode = createAsyncThunk(
    'connectionMode/fetch',
    async (_, { rejectWithValue }) => {
        try {
            // Short timeout: this is a fast local endpoint and we poll it often.
            const res = await fetchWithTimeout('/api/connection/mode', {}, 8000);
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
            state.loaded = true;
        });
    },
});

export const selectConnectionMode = (state: RootState) => state.connectionMode;

export default connectionModeSlice.reducer;
