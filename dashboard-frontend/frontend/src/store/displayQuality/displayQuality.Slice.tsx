import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getDisplayQualityUrl } from '@/assets/endpoints/api/frame';

export type DisplayQuality = 'mono' | 'gray4';

export interface DisplayQualityState {
    quality: DisplayQuality | null;
    updating: boolean;
}

const initialState: DisplayQualityState = {
    quality: null,
    updating: false,
};

export const fetchDisplayQuality = createAsyncThunk(
    'displayQuality/fetch',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getDisplayQualityUrl());
            const payload: IServerResponse & { data: { quality: DisplayQuality } } = await response.json();
            if (payload.success && payload.data) {
                return payload.data.quality;
            }
            dispatch(addAlertSnackbar(uuid(), 'Abrufen der Bildqualität fehlgeschlagen', 'error'));
            return rejectWithValue('Failed to fetch display quality');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Abrufen der Bildqualität fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const updateDisplayQuality = createAsyncThunk(
    'displayQuality/update',
    async (quality: DisplayQuality, { dispatch, rejectWithValue }) => {
        const loadingId = uuid();
        try {
            dispatch(addLoadingSnackbar(loadingId, 'Bildqualität wird geändert'));
            const response = await fetchWithTimeout(getDisplayQualityUrl(), {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ quality }),
            });
            const payload: IServerResponse = await response.json();
            if (payload.success) {
                dispatch(addAlertSnackbar(uuid(),
                    quality === 'gray4' ? '4 Graustufen ab dem nächsten Bild' : 'Schwarz-Weiß ab dem nächsten Bild',
                    'success'));
                return quality;
            }
            dispatch(addAlertSnackbar(uuid(), payload.message || 'Ändern der Bildqualität fehlgeschlagen', 'error'));
            return rejectWithValue(payload.message || 'update failed');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Ändern der Bildqualität fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        } finally {
            dispatch(removeLoadingSnackbar(loadingId));
        }
    }
);

export const displayQualitySlice = createSlice({
    name: 'displayQuality',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder
            .addCase(fetchDisplayQuality.fulfilled, (state, action) => { state.quality = action.payload; })
            .addCase(updateDisplayQuality.pending, (state) => { state.updating = true; })
            .addCase(updateDisplayQuality.fulfilled, (state, action) => {
                state.updating = false;
                state.quality = action.payload;
            })
            .addCase(updateDisplayQuality.rejected, (state) => { state.updating = false; });
    }
});

export const selectDisplayQualityState = (state: RootState) => state.displayQuality;

export default displayQualitySlice.reducer;
