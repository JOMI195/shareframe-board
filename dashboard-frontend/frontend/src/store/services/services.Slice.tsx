import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse, ServiceStatus } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getServicesUrl, getServiceRestartUrl } from '@/assets/endpoints/api/frame';

interface ServicesState {
    services: ServiceStatus[];
    loading: boolean;
    error: string | null;
    restarting: string | null; // id of the service currently being restarted
}

const initialState: ServicesState = {
    services: [],
    loading: false,
    error: null,
    restarting: null,
};

export const fetchServices = createAsyncThunk(
    'services/fetch',
    async (_arg: void, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getServicesUrl(), {
                method: 'GET',
                headers: { 'Content-Type': 'application/json', 'Cache-Control': 'no-cache' },
            });
            const payload: IServerResponse & { data: { services: ServiceStatus[] } } =
                await response.json();
            if (payload.success) return payload.data?.services ?? [];
            return rejectWithValue(payload.message || 'Failed to fetch services');
        } catch (error) {
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const restartService = createAsyncThunk(
    'services/restart',
    async (id: string, { dispatch, rejectWithValue }) => {
        const loadingId = uuid();
        try {
            dispatch(addLoadingSnackbar(loadingId, 'Starte Dienst neu'));
            const response = await fetchWithTimeout(getServiceRestartUrl(), {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ service: id }),
            });
            const payload: IServerResponse = await response.json();
            if (payload.success) {
                dispatch(addAlertSnackbar(uuid(), payload.message || 'Dienst wird neu gestartet', 'success'));
                return id;
            }
            dispatch(addAlertSnackbar(uuid(), payload.message || 'Neustart fehlgeschlagen', 'error'));
            return rejectWithValue(payload.message || 'restart failed');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Neustart fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        } finally {
            dispatch(removeLoadingSnackbar(loadingId));
        }
    }
);

const servicesSlice = createSlice({
    name: 'services',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder
            .addCase(fetchServices.pending, (state) => { state.loading = true; state.error = null; })
            .addCase(fetchServices.fulfilled, (state, action) => { state.loading = false; state.services = action.payload; })
            .addCase(fetchServices.rejected, (state, action) => { state.loading = false; state.error = action.payload as string; })
            .addCase(restartService.pending, (state, action) => { state.restarting = action.meta.arg; })
            .addCase(restartService.fulfilled, (state) => { state.restarting = null; })
            .addCase(restartService.rejected, (state) => { state.restarting = null; });
    },
});

export const selectServices = (state: RootState) => state.services.services;
export const selectServicesLoading = (state: RootState) => state.services.loading;
export const selectServiceRestarting = (state: RootState) => state.services.restarting;
export const selectService = (id: string) => (state: RootState) =>
    state.services.services.find((s) => s.id === id) ?? null;

export default servicesSlice.reducer;
