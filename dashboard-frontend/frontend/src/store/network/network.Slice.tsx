import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { boardErrorMessage } from '@/common/utils/errors/boardErrors';
import { RootState } from '@/store'; // Adjust import path as needed
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse } from '@/types';
import { fetchWithTimeout, SCRIPT_TIMEOUT_MS } from '@/common/utils/fetch';
import { getConnectionStatusUrl, getConnectionSavedNetworksUrl, getConnectionConnectUrl, getConnectionForgetUrl, getApPasswordUrl, getConnectionApplyUrl, getConnectionScanUrl } from '@/assets/endpoints/api/frame';

// Interfaces
export interface NetworkCredentials {
    ssid: string;
    password: string;
}

export interface NetworkInReach {
    ssid: string;
    signal: number;
    secured: boolean;
}

export interface NetworkState {
    // What the board's radio saw on its last station attempt; empty until it saw something.
    inReach: NetworkInReach[];
    currentConnection: string;
    savedNetworks: string[];
    loading: boolean;
    error: string | null;
    // When the board was last asked to leave its hotspot (ms); null = not in this session.
    connectRequestedAt: number | null;
}

// Initial State
const initialState: NetworkState = {
    inReach: [],
    currentConnection: 'Wird geladen …',
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
            dispatch(addAlertSnackbar(uuid(), "Netzwerke konnten nicht geladen werden", "error"));
            return rejectWithValue(errorMessage);
        }
    }
);

// Only a help to pick from: without it the name is typed.
export const fetchNetworksInReach = createAsyncThunk(
    'network/fetchNetworksInReach',
    async (_, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getConnectionScanUrl());
            const payload: IServerResponse = await response.json();
            if (payload.success) return ((payload.data as { networks?: NetworkInReach[] })?.networks ?? []);
            return rejectWithValue(payload.message);
        } catch {
            return rejectWithValue('unreachable');
        }
    }
);

// An empty password saves an open network.
export const addNetwork = createAsyncThunk(
    'network/addNetwork',
    async (credentials: NetworkCredentials, { dispatch, getState, rejectWithValue }) => {
        if (!credentials.ssid) {
            dispatch(addAlertSnackbar(uuid(), "Gib einen Netzwerknamen ein", "error"));
            return rejectWithValue('Missing credentials');
        }

        const loadingSnackbarId = uuid();

        try {
            dispatch(addLoadingSnackbar(
                loadingSnackbarId,
                'WLAN wird hinzugefügt'
            ));

            const response = await fetchWithTimeout(getConnectionConnectUrl(), {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(credentials),
            }, SCRIPT_TIMEOUT_MS);

            const data: IServerResponse = await response.json();

            if (data.success) {
                const onHotspot = (getState() as RootState).connectionMode.mode === 'ap';
                dispatch(addAlertSnackbar(uuid(), onHotspot ? "WLAN gespeichert" : "WLAN hinzugefügt", "success"));
                dispatch(fetchNetworkData());
                return credentials.ssid;
            } else {
                dispatch(addAlertSnackbar(uuid(), boardErrorMessage("WLAN konnte nicht hinzugefügt werden", response, data.message), "error"));
                return rejectWithValue('Failed to add network');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "WLAN konnte nicht hinzugefügt werden", "error"));
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
                'WLAN wird entfernt'
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
                dispatch(addAlertSnackbar(uuid(), "WLAN entfernt", "success"));
                dispatch(fetchNetworkData());
                return ssid;
            } else {
                dispatch(addAlertSnackbar(uuid(), boardErrorMessage("WLAN konnte nicht entfernt werden", response, data.message), "error"));
                return rejectWithValue('Failed to forget network');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "WLAN konnte nicht entfernt werden", "error"));
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
                'Hotspot-Passwort wird geändert'
            ));
            const response = await fetchWithTimeout(getApPasswordUrl(), {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ password }),
            }, SCRIPT_TIMEOUT_MS);

            const data: IServerResponse = await response.json();

            if (data.success) {
                dispatch(addAlertSnackbar(uuid(), "Hotspot-Passwort geändert", "success"));
                return true;
            } else {
                dispatch(addAlertSnackbar(uuid(), boardErrorMessage("Hotspot-Passwort konnte nicht geändert werden", response, data.message), "error"));
                return rejectWithValue(data.message || 'Failed to change AP password');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(uuid(), "Hotspot-Passwort konnte nicht geändert werden", "error"));
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
            dispatch(addAlertSnackbar(uuid(), boardErrorMessage("Verbindung konnte nicht gestartet werden", response, data.message), "error"));
            return rejectWithValue(data.message || 'Failed to connect');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), "Verbindung konnte nicht gestartet werden", "error"));
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

        builder.addCase(fetchNetworksInReach.fulfilled, (state, action) => {
            state.inReach = action.payload;
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