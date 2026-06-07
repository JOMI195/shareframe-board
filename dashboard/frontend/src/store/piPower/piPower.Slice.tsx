import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState, AppDispatch } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getClearDisplayUrl, getRestartPiUrl, getShutdownPiUrl } from '@/assets/endpoints/api/frame';
import { showLoadingWall } from '../loadingWall/loadingWall.Slice';
import { toggleSlideshowThunk } from '../slideshowOperation/slideshowOperation.Slice';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

export interface PiPowerState {
}

const initialState: PiPowerState = {

};

const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

export const restartPi = createAsyncThunk<void, (path: string) => void, { state: RootState, dispatch: AppDispatch }>(
    'piPower/restartPi',
    async (navigate, { dispatch, rejectWithValue }) => {
        const snackbarId = uuid();
        try {
            dispatch(addLoadingSnackbar(snackbarId, "Starte Neustartprozess"));

            const response = await fetchWithTimeout(getRestartPiUrl(), { method: 'POST', });
            const payload: IServerResponse = await response.json();

            if (payload.success) {
                dispatch(addAlertSnackbar(uuid(), "Neustartprozess gestartet", "success"));
                navigate(getHomeUrl());
                dispatch(showLoadingWall("Der Bilderrahmen wird neu gestartet. Während dieser Zeit ist das Dashboard nicht verfügbar. Lade diese Seite in ein paar Minuten erneut bis es wieder verfügbar ist."));
            } else {
                dispatch(addAlertSnackbar(uuid(), "Neustartprozess fehlgeschlagen", "error"));
                return rejectWithValue('Neustartprozess fehlgeschlagen');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Neustartprozess fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(snackbarId));
        }
    }
);

export const shutdownPi = createAsyncThunk<void, (path: string) => void, { state: RootState, dispatch: AppDispatch }>(
    'piPower/shutdownPi',
    async (navigate, { dispatch, rejectWithValue, getState }) => {
        const snackbarId = uuid();
        try {
            dispatch(addLoadingSnackbar(snackbarId, "Fahre Bilderrahmen herunter. Dies kann einige Minuten in Anpruch nehmen."));

            const state = getState() as RootState;

            if (!state.slideshowStatus.isActive) {
                await fetchWithTimeout(getClearDisplayUrl(), { method: 'POST' });
            } else {
                await dispatch(toggleSlideshowThunk());
            }

            const response = await fetchWithTimeout(getShutdownPiUrl(), { method: 'POST', });
            const payload: IServerResponse = await response.json();

            if (payload.success) {
                await delay(90000);
                navigate(getHomeUrl());
                dispatch(showLoadingWall("Der Bilderrahmen wurde erfolgreich heruntergefahren. Das Dashboard ist nun nicht mehr verfügbar. Um ihn neu zu starten musst du die Stromzufuhr unterbrechen und wieder herstellen."));
            } else {
                dispatch(addAlertSnackbar(uuid(), "Herunterfahrprozess fehlgeschlagen", "error"));
                return rejectWithValue('Herunterfahrprozess fehlgeschlagen');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Herunterfahrprozess fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(snackbarId));
        }
    }
);

export const PiPowerSlice = createSlice({
    name: 'piPower',
    initialState,
    reducers: {}
});

export const selectPiPowerState = (state: RootState) => state.piPower;

export default PiPowerSlice.reducer;