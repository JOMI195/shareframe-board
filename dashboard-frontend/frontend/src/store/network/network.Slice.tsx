import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store'; // Adjust import path as needed
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getConnectionStatusUrl, getConnectionSavedNetworksUrl, getConnectionConnectUrl, getConnectionForgetUrl, getApPasswordUrl, getConnectionApplyUrl } from '@/assets/endpoints/api/frame';

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
    // When the board was last asked to leave its hotspot (ms); null = not in this session.
    connectRequestedAt: number | null;
}

// Initial State
const initialState: NetworkState = {
    currentConnection: 'Loading...',
    savedNetworks: [],
    loading: false,
    error: null,
    connectRequestedAt: null
};

// Async Thunks
export const fetchNetworkData = createAsyncThunk(
    'network/fetchNetworkData',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            // Current connection
            const connResponse = await fetchWithTimeout(getConnectionStatusUrl());
            const connPayload = await connResponse.json();

            // Saved networks
            const networksResponse = await fetchWithTimeout(getConnectionSavedNetworksUrl());
            const networksPayload = await networksResponse.json();

            if (connPayload.success && networksPayload.success) {
                return {
                    currentConnection: connPayload.data.connection_name,
                    savedNetworks: networksPayload.data.networks
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
    async (credentials: NetworkCredentials, { dispatch, getState, rejectWithValue }) => {
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

            const response = await fetchWithTimeout(getConnectionConnectUrl(), {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(credentials),
            });

            const data: IServerResponse = await response.json();

            if (data.success) {
                const onHotspot = (getState() as RootState).connectionMode.mode === 'ap';
                dispatch(addAlertSnackbar(uuid(), onHotspot
                    ? "Netzwerk gespeichert – mit „Jetzt verbinden“ wechselt der Rahmen ins WLAN"
                    : "Netzwerk erfolgreich hinzugefügt", "success"));
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
            const response = await fetchWithTimeout(getConnectionForgetUrl(), {
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

export const changeApPassword = createAsyncThunk(
    'network/changeApPassword',
    async (password: string, { dispatch, rejectWithValue }) => {
        const loadingSnackbarId = uuid();

        try {
            dispatch(addLoadingSnackbar(
                loadingSnackbarId,
                'AP-Passwort wird geändert'
            ));
            const response = await fetchWithTimeout(getApPasswordUrl(), {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ password }),
            });

            const data: IServerResponse = await response.json();

            if (data.success) {
                dispatch(addAlertSnackbar(uuid(), "AP-Passwort erfolgreich geändert", "success"));
                return true;
            } else {
                dispatch(addAlertSnackbar(uuid(), data.message || "Ändern des AP-Passworts fehlgeschlagen", "error"));
                return rejectWithValue(data.message || 'Failed to change AP password');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Ändern des AP-Passworts fehlgeschlagen", "error"));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(loadingSnackbarId));
        }
    }
);

// Makes the board leave its hotspot and try the saved networks.
export const connectToSavedNetworks = createAsyncThunk(
    'network/connectToSavedNetworks',
    async (_, { dispatch, rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getConnectionApplyUrl(), { method: 'POST' });
            const data: IServerResponse = await response.json();
            if (data.success) {
                return true;
            }
            dispatch(addAlertSnackbar(uuid(), "Verbinden fehlgeschlagen", "error"));
            return rejectWithValue(data.message || 'Failed to connect');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), "Verbinden fehlgeschlagen", "error"));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
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

        builder.addCase(connectToSavedNetworks.fulfilled, (state) => {
            state.connectRequestedAt = Date.now();
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
    }
});

// Selectors
export const selectNetworkState = (state: RootState) => state.network;

export default networkSlice.reducer;