import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';

// Interfaces
export interface FrameInfo {
    public_serial_number: string;
    version: string;
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
            const response = await fetchWithTimeout('/api/frame/infos');
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