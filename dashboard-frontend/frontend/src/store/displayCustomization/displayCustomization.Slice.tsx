import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getDisplayCustomizationUrl } from '@/assets/endpoints/api/frame';

interface DisplayCustomization {
    show_sender: boolean;
}

export interface DisplayCustomizationState {
    showSender: boolean;
    isFetching: boolean;
    isUpdating: boolean;
}

const initialState: DisplayCustomizationState = {
    showSender: false,
    isFetching: false,
    isUpdating: false,
};

export const fetchDisplayCustomization = createAsyncThunk(
    'displayCustomization/fetch',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getDisplayCustomizationUrl());
            const payload: IServerResponse & { data: DisplayCustomization } = await response.json();
            if (payload.success && payload.data) {
                return payload.data;
            }
            throw new Error(payload.message || 'Failed to fetch customization');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Abrufen der Anpassungen fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const updateShowSender = createAsyncThunk(
    'displayCustomization/updateShowSender',
    async (showSender: boolean, { dispatch, rejectWithValue }) => {
        const actionId = uuid();
        dispatch(addLoadingSnackbar(actionId, 'Anpassung wird gespeichert'));
        try {
            const response = await fetchWithTimeout(getDisplayCustomizationUrl(), {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ show_sender: showSender }),
            });
            const payload: IServerResponse = await response.json();
            if (!payload.success) {
                throw new Error(payload.message || 'Failed to update customization');
            }
            dispatch(addAlertSnackbar(
                uuid(),
                showSender
                    ? 'Der Name des Absenders wird ab dem nächsten Bildwechsel angezeigt'
                    : 'Der Name des Absenders wird ab dem nächsten Bildwechsel ausgeblendet',
                'success'
            ));
            return showSender;
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Speichern der Anpassung fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        } finally {
            dispatch(removeLoadingSnackbar(actionId));
        }
    }
);

export const displayCustomizationSlice = createSlice({
    name: 'displayCustomization',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder.addCase(fetchDisplayCustomization.pending, (state) => {
            state.isFetching = true;
        });
        builder.addCase(fetchDisplayCustomization.fulfilled, (state, action) => {
            state.isFetching = false;
            state.showSender = action.payload.show_sender;
        });
        builder.addCase(fetchDisplayCustomization.rejected, (state) => {
            state.isFetching = false;
        });
        builder.addCase(updateShowSender.pending, (state) => {
            state.isUpdating = true;
        });
        builder.addCase(updateShowSender.fulfilled, (state, action) => {
            state.isUpdating = false;
            state.showSender = action.payload;
        });
        builder.addCase(updateShowSender.rejected, (state) => {
            state.isUpdating = false;
        });
    }
});

export const selectDisplayCustomization = (state: RootState) => state.displayCustomization;

export default displayCustomizationSlice.reducer;
