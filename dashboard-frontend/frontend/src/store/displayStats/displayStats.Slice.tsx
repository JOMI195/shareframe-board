import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';

// Mirrors GET /api/frame/display/stats `data` (DisplayManager::healthSnapshot).
// Counter keys are absent until the panel has been used at least once; the
// derived fields (health/wear_percent/...) are always present.
export type DisplayHealth = 'ok' | 'degraded' | 'failed';

export interface DisplayStats {
    // Persisted wear counters
    epd_refresh_total?: number;
    epd_image_refresh_total?: number;
    epd_clear_total?: number;
    epd_poweron_total?: number;
    epd_refresh_fail_total?: number;
    epd_poweron_fail_total?: number;
    epd_busy_ms_total?: number;
    epd_last_refresh_ms?: number;
    epd_last_refresh_at?: number;
    epd_first_use_at?: number;
    app_boot_total?: number;
    // Derived at read time
    consecutive_failures?: number;
    rated_refreshes?: number;
    wear_percent?: number;
    health?: DisplayHealth;
}

export interface DisplayStatsState {
    displayStats: DisplayStats | null;
    loading: boolean;
}

const initialState: DisplayStatsState = {
    displayStats: null,
    loading: false,
};

export const fetchDisplayStats = createAsyncThunk(
    'displayStats/fetchDisplayStats',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout('/api/frame/display/stats');
            const payload: IServerResponse & { data: DisplayStats } = await response.json();

            if (payload.success && payload.data) {
                return payload.data;
            } else {
                dispatch(addAlertSnackbar(uuid(), "Abrufen der Display-Statistik fehlgeschlagen", "error"));
                return rejectWithValue('Failed to fetch display stats');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Abrufen der Display-Statistik fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        }
    }
);

export const displayStatsSlice = createSlice({
    name: 'displayStats',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder.addCase(fetchDisplayStats.pending, (state) => {
            state.loading = true;
        });
        builder.addCase(fetchDisplayStats.fulfilled, (state, action) => {
            state.loading = false;
            state.displayStats = action.payload;
        });
        builder.addCase(fetchDisplayStats.rejected, (state) => {
            state.loading = false;
        });
    }
});

export const selectDisplayStatsState = (state: RootState) => state.displayStats;

export default displayStatsSlice.reducer;
