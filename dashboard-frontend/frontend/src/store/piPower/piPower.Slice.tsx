import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState, AppDispatch } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getClearDisplayUrl, getRestartPiUrl, getShutdownPiUrl, getSystemHealthUrl } from '@/assets/endpoints/api/frame';
import { showLoadingWall } from '../loadingWall/loadingWall.Slice';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

// This slice holds no state; power actions are fire-and-forget async thunks.
export type PiPowerState = Record<string, never>;

const initialState: PiPowerState = {};

const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

// A power command (reboot/shutdown) tears the server down, so its HTTP response
// often never arrives - the connection just drops. That is the expected happy
// path, NOT a failure. We only treat an explicit `success: false` body as a real
// rejection; any network/abort error means "the box is going away as asked".
const sendPowerCommand = async (url: string): Promise<'accepted' | 'rejected'> => {
    try {
        const response = await fetchWithTimeout(url, { method: 'POST' }, 8000);
        const payload: IServerResponse = await response.json();
        return payload.success ? 'accepted' : 'rejected';
    } catch {
        return 'accepted';
    }
};

const isReachable = async (perRequestTimeoutMs: number): Promise<boolean> => {
    try {
        const response = await fetchWithTimeout(getSystemHealthUrl(), {}, perRequestTimeoutMs);
        if (!response.ok) return false;
        const payload = await response.json();
        return !!payload?.data?.running;
    } catch {
        return false;
    }
};

// Poll /api/system/health until the board reaches the desired state ('up' or
// 'down') for `consecutive` checks in a row, or `maxWaitMs` elapses. Returns
// true if the state was reached, false on timeout. Replaces blind fixed delays.
const pollHealth = async (
    target: 'up' | 'down',
    { maxWaitMs = 120000, intervalMs = 3000, perRequestTimeoutMs = 3000, consecutive = 2 } = {}
): Promise<boolean> => {
    const start = Date.now();
    let streak = 0;
    while (Date.now() - start < maxWaitMs) {
        const reachable = await isReachable(perRequestTimeoutMs);
        const hit = target === 'up' ? reachable : !reachable;
        streak = hit ? streak + 1 : 0;
        if (streak >= consecutive) return true;
        await delay(intervalMs);
    }
    return false;
};

export const restartPi = createAsyncThunk<void, (path: string) => void, { state: RootState, dispatch: AppDispatch }>(
    'piPower/restartPi',
    async (navigate, { dispatch, rejectWithValue }) => {
        const snackbarId = uuid();
        try {
            dispatch(addLoadingSnackbar(snackbarId, "Starte Neustartprozess"));

            const result = await sendPowerCommand(getRestartPiUrl());
            if (result === 'rejected') {
                dispatch(addAlertSnackbar(uuid(), "Neustartprozess fehlgeschlagen", "error"));
                return rejectWithValue('Neustartprozess fehlgeschlagen');
            }

            dispatch(addAlertSnackbar(uuid(), "Neustartprozess gestartet", "success"));
            navigate(getHomeUrl());
            dispatch(showLoadingWall("Der Bilderrahmen wird neu gestartet. Bitte warte einen Moment – die Seite lädt automatisch neu, sobald er wieder verfügbar ist."));

            // Wait for the board to drop (reboot started), then for it to come
            // back, then reload onto a working dashboard automatically.
            await pollHealth('down', { maxWaitMs: 60000, consecutive: 1 });
            const backUp = await pollHealth('up', { maxWaitMs: 180000, consecutive: 1 });
            if (backUp) {
                window.location.reload();
            }
            // If it never came back in time the wall stays up; the user can reload manually.
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
    async (navigate, { dispatch, rejectWithValue }) => {
        const snackbarId = uuid();
        try {
            dispatch(addLoadingSnackbar(snackbarId, "Fahre Bilderrahmen herunter. Dies kann einige Minuten in Anspruch nehmen."));

            // Best-effort: clear the panel so it isn't left mid-cycle. Never block
            // the shutdown on it - the slideshow stops on its own when the process
            // dies a moment later.
            try {
                await fetchWithTimeout(getClearDisplayUrl(), { method: 'POST' }, 8000);
            } catch { /* ignore */ }

            const result = await sendPowerCommand(getShutdownPiUrl());
            if (result === 'rejected') {
                dispatch(addAlertSnackbar(uuid(), "Herunterfahrprozess fehlgeschlagen", "error"));
                return rejectWithValue('Herunterfahrprozess fehlgeschlagen');
            }

            navigate(getHomeUrl());
            dispatch(showLoadingWall("Der Bilderrahmen wird heruntergefahren …"));

            // Show the terminal "how to power back on" message only once the board
            // has actually stopped answering, not after a blind fixed delay.
            await pollHealth('down', { maxWaitMs: 120000, consecutive: 2 });
            dispatch(showLoadingWall("Der Bilderrahmen wurde heruntergefahren und ist ausgeschaltet. Zum Einschalten die Stromzufuhr kurz trennen und wieder herstellen – er startet dann automatisch."));
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
