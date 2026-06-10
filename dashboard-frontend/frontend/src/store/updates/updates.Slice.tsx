import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getLatestReleaseUrl, getPerformUpdateUrl, getUpdateStatusUrl, getUpdateHistoryUrl } from '@/assets/endpoints/api/frame';
import { showLoadingWall } from '../loadingWall/loadingWall.Slice';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

interface Release {
    version: string;
    release_notes: string;
    release_date: string;
    criticality: string;
}

export interface UpdateHistoryEntry {
    timestamp: string;
    from_version: string;
    to_version: string;
    result: 'committed' | 'rolled-back' | 'install-failed' | string;
    error: string;
}

export interface UpdateStatus {
    phase: 'idle' | 'checking' | 'downloading' | 'installing' | 'awaiting-reboot' | 'failed' | string;
    progress: number;
    error: string;
    current_version: string;
    target_version: string;
    booted_slot: string;
    committed_slot: string;
    pending_slot: string;
    committed: boolean;
    last_result?: UpdateHistoryEntry;
}

export interface UpdatesState {
    latest_release: Release | null;
    update_status: UpdateStatus | null;
    history: UpdateHistoryEntry[];
    loading: boolean;
}

const initialState: UpdatesState = {
    latest_release: null,
    update_status: null,
    history: [],
    loading: false,
};

export const fetchLatestRelease = createAsyncThunk(
    'updates/fetchLatestRelease',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getLatestReleaseUrl());
            const payload: IServerResponse & { data: Release } = await response.json();

            if (payload.success && payload.data) {
                return payload.data;
            } else {
                dispatch(addAlertSnackbar(uuid(), "Suche nach neuster Version fehlgeschlagen", "error"));
                return rejectWithValue('Failed to fetch latest release');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Suche nach neuer Version fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        }
    }
);

export const performUpdate = createAsyncThunk(
    'updates/performUpdate',
    async (navigate: (path: string) => void, { dispatch, rejectWithValue }) => {
        const snackbarId = uuid();
        try {
            dispatch(addLoadingSnackbar(snackbarId, "Starte Updateprozess"));

            const response = await fetchWithTimeout(getPerformUpdateUrl(), { method: 'POST', });
            const payload: IServerResponse = await response.json();

            if (payload.success) {
                dispatch(addAlertSnackbar(uuid(), "Updateprozess gestartet", "success"));
                navigate(getHomeUrl());
                dispatch(showLoadingWall("Die Updates werden installiert. Während dieser Zeit ist das Dashboard nicht verfügbar. Die Applikation wird anschließend neu gestartet. Lade diese Seite in ein paar Minuten einfach ein paar Mal erneut, bis sie wieder verfügbar ist."));
            } else {
                dispatch(addAlertSnackbar(uuid(), "Updateprozess fehlgeschlagen", "error"));
                return rejectWithValue('Failed start update process');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Updateprozess fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(snackbarId));
        }
    }
);

export const fetchUpdateStatus = createAsyncThunk(
    'updates/fetchUpdateStatus',
    async (_, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getUpdateStatusUrl());
            const payload: IServerResponse & { data: UpdateStatus } = await response.json();
            if (payload.success && payload.data) {
                return payload.data;
            }
            return rejectWithValue('Failed to fetch update status');
        } catch (error) {
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const fetchUpdateHistory = createAsyncThunk(
    'updates/fetchUpdateHistory',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getUpdateHistoryUrl());
            const payload: IServerResponse & { data: { history: UpdateHistoryEntry[] } } = await response.json();
            if (payload.success && payload.data) {
                return payload.data.history;
            }
            dispatch(addAlertSnackbar(uuid(), "Update-Verlauf konnte nicht geladen werden", "error"));
            return rejectWithValue('Failed to fetch update history');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), "Update-Verlauf konnte nicht geladen werden", "error"));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const UpdatesSlice = createSlice({
    name: 'updates',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        // Fetch Frame Infos
        builder.addCase(fetchLatestRelease.pending, (state) => {
            state.loading = true;
        });
        builder.addCase(fetchLatestRelease.fulfilled, (state, action) => {
            state.latest_release = action.payload;
            state.loading = false;
        });
        builder.addCase(fetchLatestRelease.rejected, (state) => {
            state.loading = false;
        });
        builder.addCase(fetchUpdateStatus.fulfilled, (state, action) => {
            state.update_status = action.payload;
        });
        builder.addCase(fetchUpdateHistory.fulfilled, (state, action) => {
            state.history = action.payload;
        });
    }
});

export const selectUpdatesState = (state: RootState) => state.updates;

export default UpdatesSlice.reducer;