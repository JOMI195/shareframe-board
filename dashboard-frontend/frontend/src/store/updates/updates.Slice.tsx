import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getLatestReleaseUrl, getPerformUpdateUrl } from '@/assets/endpoints/api/frame';
import { showLoadingWall } from '../loadingWall/loadingWall.Slice';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

interface Release {
    version: string;
    download_url: string;
    checksum: string;
    release_notes: string;
    release_date: string;
    criticality: string;
}

export interface UpdatesState {
    latest_release: Release | null;
    loading: boolean;
}

const initialState: UpdatesState = {
    latest_release: null,
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
    }
});

export const selectUpdatesState = (state: RootState) => state.updates;

export default UpdatesSlice.reducer;