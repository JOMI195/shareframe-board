import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';

// Interfaces — mirrors GET /api/system/info `data`. Metric fields are optional:
// the board script omits a key when its source is absent.
export interface FrameInfo {
    serial_number: string;
    version: string;
    hostname?: string;
    kernel?: string;
    ip_wlan0?: string;
    ip_usb0?: string;
    time_iso?: string;
    uptime_seconds?: number;
    ram_total_bytes?: number;
    ram_available_bytes?: number;
    storage_data_total_bytes?: number;
    storage_data_free_bytes?: number;
    storage_root_total_bytes?: number;
    storage_root_free_bytes?: number;
    cpu_temp_celsius?: number;
    load_1?: number;
    load_5?: number;
    load_15?: number;
    cpu_usage_percent?: number;
    wlan_ssid?: string;
    wlan_signal_dbm?: number;
    wlan_connected?: boolean;
    boot_count?: number;
}

export interface FrameInfoState {
    frameInfo: FrameInfo | null;
    loading: boolean;
}

// Initial State
const initialState: FrameInfoState = {
    frameInfo: null,
    loading: false,
};

// Async Thunk
export const fetchFrameInfos = createAsyncThunk(
    'frameInfo/fetchFrameInfos',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout('/api/system/info');
            const payload: IServerResponse & { data: FrameInfo } = await response.json();

            if (payload.success && payload.data) {
                return payload.data;
            } else {
                dispatch(addAlertSnackbar(uuid(), "Abrufen der Frame-Informationen fehlgeschlagen", "error"));
                return rejectWithValue('Failed to fetch frame infos');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Abrufen der Frame-Informationen fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        }
    }
);

// Slice
export const frameInfoSlice = createSlice({
    name: 'frameInfo',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        // Fetch Frame Infos
        builder.addCase(fetchFrameInfos.pending, (state) => {
            state.loading = true;
        });
        builder.addCase(fetchFrameInfos.fulfilled, (state, action) => {
            state.loading = false;
            state.frameInfo = action.payload;
        });
        builder.addCase(fetchFrameInfos.rejected, () => {
        });
    }
});

// Selectors
export const selectFrameInfoState = (state: RootState) => state.frameInfo;

export default frameInfoSlice.reducer;