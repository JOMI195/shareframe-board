import { createSlice, PayloadAction } from '@reduxjs/toolkit';
import { RootState } from '..';
import { AlertColor } from '@mui/material';

export interface SnackbarItem {
    id: string;
    message: string;
    autoHideDuration?: number;
}

export interface AlertSnackbarItem extends SnackbarItem {
    severity: AlertColor;
}

export interface SnackbarsState {
    snackbars: {
        alerts: AlertSnackbarItem[];
        loading: SnackbarItem[];
    };
}

const initialState: SnackbarsState = {
    snackbars: {
        alerts: [],
        loading: [],
    },
};

const MAX_ALERT_SNACKBARS = 5;

const snackbarsSlice = createSlice({
    name: 'snackbars',
    initialState,
    reducers: {
        alertSnackbarAdded: (state, action: PayloadAction<AlertSnackbarItem>) => {
            if (state.snackbars.alerts.length >= MAX_ALERT_SNACKBARS) {
                state.snackbars.alerts.shift();
            }
            state.snackbars.alerts.push(action.payload);
        },
        alertSnackbarRemoved: (state, action: PayloadAction<string>) => {
            state.snackbars.alerts = state.snackbars.alerts.filter(
                snackbar => snackbar.id !== action.payload
            );
        },
        loadingSnackbarAdded: (state, action: PayloadAction<SnackbarItem>) => {
            state.snackbars.loading.push(action.payload);
        },
        loadingSnackbarRemoved: (state, action: PayloadAction<string>) => {
            state.snackbars.loading = state.snackbars.loading.filter(
                snackbar => snackbar.id !== action.payload
            );
        },
        clearAllSnackbars(state) {
            state.snackbars.alerts = [];
            state.snackbars.loading = [];
        },
        clearAllLoadingSnackbars(state) {
            state.snackbars.loading = [];
        },
    },
});

export const addAlertSnackbar = (
    id: string,
    message: string,
    severity: AlertColor,
    autoHideDuration?: number
) => ({
    type: alertSnackbarAdded.type,
    payload: { id, message, severity, autoHideDuration },
});

export const removeAlertSnackbar = (id: string) => ({
    type: alertSnackbarRemoved.type,
    payload: id,
});

export const addLoadingSnackbar = (
    id: string,
    message: string,
    autoHideDuration?: number
) => ({
    type: loadingSnackbarAdded.type,
    payload: { id, message, autoHideDuration },
});

export const removeLoadingSnackbar = (id: string) => ({
    type: loadingSnackbarRemoved.type,
    payload: id,
});

export const removeAllLoadingSnackbars = () => ({
    type: clearAllLoadingSnackbars.type,
});

export const removeAllSnackbars = () => ({
    type: clearAllSnackbars.type,
});

export const {
    alertSnackbarAdded,
    alertSnackbarRemoved,
    loadingSnackbarAdded,
    loadingSnackbarRemoved,
    clearAllSnackbars,
    clearAllLoadingSnackbars
} = snackbarsSlice.actions;

export const getSnackbars = (state: RootState) => state.snackbars.snackbars;

export default snackbarsSlice.reducer;