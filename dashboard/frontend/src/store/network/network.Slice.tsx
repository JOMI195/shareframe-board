import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store'; // Adjust import path as needed
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getConnectionRenameUrl } from '@/assets/endpoints/api/frame';

// Interfaces
export interface NetworkCredentials {
    ssid: string;
    password: string;
}

export interface NetworkState {
    currentConnection: string;
    savedNetworks: string[];
    loading: boolean;
    error: string | null;
}

// Initial State
const initialState: NetworkState = {
    currentConnection: 'Loading...',
    savedNetworks: [],
    loading: false,
    error: null
};

// Async Thunks
export const fetchNetworkData = createAsyncThunk(
    'network/fetchNetworkData',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            // Fetch current connection
            const connResponse = await fetchWithTimeout('/api/connection/current-connection');
            const connData = await connResponse.json();

            // Fetch saved networks
            const networksResponse = await fetchWithTimeout('/api/connection/saved-networks');
            const networksData = await networksResponse.json();

            if (connData.success && networksData.success) {
                return {
                    currentConnection: connData.connection,
                    savedNetworks: networksData.networks
                };
            } else {
                return rejectWithValue('Failed to fetch network data');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Aktualisieren fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        }
    }
);

export const addNetwork = createAsyncThunk(
    'network/addNetwork',
    async (credentials: NetworkCredentials, { dispatch, rejectWithValue }) => {
        if (!credentials.ssid || !credentials.password) {
            dispatch(addAlertSnackbar(uuid(), "SSID und Passwort benötigt", "error"));
            return rejectWithValue('Missing credentials');
        }

        const loadingSnackbarId = uuid();

        try {
            dispatch(addLoadingSnackbar(
                loadingSnackbarId,
                'Netzwerk hinzufügen'
            ));

            const response = await fetchWithTimeout('/api/connection/connect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(credentials),
            });

            const data: IServerResponse = await response.json();

            if (data.success) {
                dispatch(addAlertSnackbar(uuid(), "Netzwerk erfolgreich hinzugefügt", "success"));
                dispatch(fetchNetworkData());
                return credentials.ssid;
            } else {
                dispatch(addAlertSnackbar(uuid(), "Hinzufügen des Netzwerks fehlgeschlagen", "error"));
                return rejectWithValue('Failed to add network');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Hinzufügen des Netzwerks fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(loadingSnackbarId));
        }
    }
);

export const forgetNetwork = createAsyncThunk(
    'network/forgetNetwork',
    async (ssid: string, { dispatch, rejectWithValue }) => {
        const loadingSnackbarId = uuid();

        try {
            dispatch(addLoadingSnackbar(
                loadingSnackbarId,
                'Netzwerk entfernen'
            ));
            const response = await fetchWithTimeout('/api/connection/forget', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ ssid }),
            });

            const data: IServerResponse = await response.json();

            if (data.success) {
                dispatch(addAlertSnackbar(uuid(), "Netzwerk erfolgreich entfernt", "success"));
                dispatch(fetchNetworkData());
                return ssid;
            } else {
                dispatch(addAlertSnackbar(uuid(), "Entfernen des Netzwerks fehlgeschlagen", "error"));
                return rejectWithValue('Failed to forget network');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Entfernen des Netzwerks fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(loadingSnackbarId));
        }
    }
);

export const renameNetwork = createAsyncThunk(
    'network/renameNetwork',
    async (
        { oldName, newName }: { oldName: string; newName: string },
        { dispatch, rejectWithValue }
    ) => {
        const loadingSnackbarId = uuid();

        try {
            dispatch(
                addLoadingSnackbar(
                    loadingSnackbarId,
                    'Netzwerk umbenennen'
                )
            );

            const response = await fetchWithTimeout(getConnectionRenameUrl(), {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ oldName, newName }),
            });

            const data: IServerResponse = await response.json();

            if (data.success) {
                dispatch(addAlertSnackbar(uuid(), "Netzwerk erfolgreich umbenannt", "success"));
                dispatch(fetchNetworkData());
                return { oldName, newName };
            } else {
                dispatch(addAlertSnackbar(uuid(), "Umbenennen des Netzwerks fehlgeschlagen", "error"));
                return rejectWithValue(data.message || 'Failed to rename network');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Umbenennen des Netzwerks fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(loadingSnackbarId));
        }
    }
);

// Slice
export const networkSlice = createSlice({
    name: 'network',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        // Fetch Network Data
        builder.addCase(fetchNetworkData.pending, (state) => {
            state.loading = true;
            state.error = null;
        });
        builder.addCase(fetchNetworkData.fulfilled, (state, action) => {
            state.loading = false;
            state.currentConnection = action.payload.currentConnection;
            state.savedNetworks = action.payload.savedNetworks;
        });
        builder.addCase(fetchNetworkData.rejected, (state, action) => {
            state.loading = false;
            state.error = action.payload as string;
        });

        // Add Network
        builder.addCase(addNetwork.pending, (state) => {
            state.loading = true;
            state.error = null;
        });
        builder.addCase(addNetwork.fulfilled, (state) => {
            state.loading = false;
        });
        builder.addCase(addNetwork.rejected, (state, action) => {
            state.loading = false;
            state.error = action.payload as string;
        });

        // Forget Network
        builder.addCase(forgetNetwork.pending, (state) => {
            state.loading = true;
            state.error = null;
        });
        builder.addCase(forgetNetwork.fulfilled, (state) => {
            state.loading = false;
        });
        builder.addCase(forgetNetwork.rejected, (state, action) => {
            state.loading = false;
            state.error = action.payload as string;
        });

        // Rename Network
        builder.addCase(renameNetwork.pending, (state) => {
            state.loading = true;
            state.error = null;
        });
        builder.addCase(renameNetwork.fulfilled, (state) => {
            state.loading = false;
        });
        builder.addCase(renameNetwork.rejected, (state, action) => {
            state.loading = false;
            state.error = action.payload as string;
        });
    }
});

// Selectors
export const selectNetworkState = (state: RootState) => state.network;

export default networkSlice.reducer;