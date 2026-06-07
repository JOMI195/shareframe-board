import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '@/store';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { IServerResponse, LogsData, LogsRequestParams } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getFrameLogsUrl } from '@/assets/endpoints/api/frame';

interface LogsState {
    logsData: LogsData | null;
    loading: boolean;
    error: string | null;
    lastFetchTimestamp: number | null;
    cachedParams: string | null;
}

const initialState: LogsState = {
    logsData: null,
    loading: false,
    error: null,
    lastFetchTimestamp: null,
    cachedParams: null
};

const CACHE_DURATION = 30 * 1000;

export const fetchFrameLogs = createAsyncThunk(
    'frameLogs/fetchServiceLogs',
    async (params: LogsRequestParams, { dispatch, rejectWithValue, getState }) => {
        const loadingBarId = uuid();

        try {
            dispatch(addLoadingSnackbar(
                loadingBarId,
                'Lade Protokolle'
            ));

            const state = getState() as RootState;
            const currentTime = Date.now();
            const paramsString = JSON.stringify(params);

            if (
                state.frameLogs.logsData &&
                state.frameLogs.lastFetchTimestamp &&
                state.frameLogs.cachedParams === paramsString &&
                currentTime - state.frameLogs.lastFetchTimestamp < CACHE_DURATION
            ) {
                return state.frameLogs.logsData;
            }

            const queryParams = new URLSearchParams();

            if (params.service_name) queryParams.append('service_name', params.service_name);

            if (params.since_timestamp) {
                queryParams.append('since_timestamp', params.since_timestamp);
            }

            if (params.lines) queryParams.append('lines', params.lines.toString());

            const url = `${getFrameLogsUrl()}?${queryParams.toString()}`;

            const response = await fetchWithTimeout(url, {
                method: 'GET',
                headers: {
                    'Content-Type': 'application/json',
                    'Cache-Control': 'no-cache'
                }
            });

            const payload: IServerResponse & {
                service: string;
                period: string;
                timestamp: string;
                log_count: number;
                logs: string[];
            } = await response.json();

            if (payload.success) {
                return {
                    service: payload.service,
                    period: payload.period,
                    timestamp: payload.timestamp,
                    log_count: payload.log_count,
                    logs: payload.logs,
                    lastFetched: currentTime
                };
            } else {
                dispatch(addAlertSnackbar(
                    uuid(),
                    `Abrufen der Logs fehlgeschlagen`,
                    "error"
                ));
                return rejectWithValue(payload.message || 'Failed to fetch logs');
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error';
            dispatch(addAlertSnackbar(
                uuid(),
                `Abrufen der Logs fehlgeschlagen`,
                "error"
            ));
            return rejectWithValue(errorMessage);
        } finally {
            dispatch(removeLoadingSnackbar(loadingBarId));
        }
    }
);

export const frameLogsSlice = createSlice({
    name: 'frameLogs',
    initialState,
    reducers: {
        clearLogs: (state) => {
            state.logsData = null;
            state.error = null;
            state.lastFetchTimestamp = null;
            state.cachedParams = null;
        }
    },
    extraReducers: (builder) => {
        builder.addCase(fetchFrameLogs.pending, (state) => {
            state.loading = true;
            state.error = null;
        });
        builder.addCase(fetchFrameLogs.fulfilled, (state, action) => {
            state.loading = false;
            state.logsData = action.payload;
            state.error = null;
            state.lastFetchTimestamp = Date.now();
            state.cachedParams = JSON.stringify(action.meta.arg);
        });
        builder.addCase(fetchFrameLogs.rejected, (state, action) => {
            state.loading = false;
            state.error = action.payload as string;
        });
    }
});

export const { clearLogs } = frameLogsSlice.actions;

export const selectLogsState = (state: RootState) => state.frameLogs;
export const selectLogsData = (state: RootState) => state.frameLogs.logsData;
export const selectLogsLoading = (state: RootState) => state.frameLogs.loading;
export const selectLogsError = (state: RootState) => state.frameLogs.error;

export default frameLogsSlice.reducer;