import { createSlice, PayloadAction } from '@reduxjs/toolkit';
import { AppDispatch, RootState } from '..';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '../snackbars/snackbars.Slice';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { IServerResponse } from '@/types';
import { addTimer, resetTimer, startTimer } from '../timers/timers.Slice';
import { fetchSlideshowImages } from '../slideshowImages/slideshowImages.Slice';
import { getClearDisplayUrl, getSlideshowIntervalUrl, getSlideshowNightModeUrl, getSkipSlideshowImageUrl, getShowSlideshowImageUrl, getSlideshowStatusUrl, getSlideshowUrl } from '@/assets/endpoints/api/frame';


const MAX_OPERATION_WAIT_TIME = 10 * 60 * 1000; // 10 minutes

// Types for Slideshow Operation State
export type NightStyle = 'slower' | 'hold' | 'pause';

interface SlideshowOperationState {
    isToggling: boolean;
    isClearingDisplay: boolean;
    isSkippingImage: boolean;
    isShowingImage: boolean;
    isUpdatingInterval: boolean;
    isFetchingInterval: boolean;
    isUpdatingNightMode: boolean;
    isFetchingNightMode: boolean;
    displayImagesIntervalMins: number;
    nightModeEnabled: boolean;
    nightModeActiveNow: boolean;
    nightStartHour: number;
    nightEndHour: number;
    nightIntervalMins: number;
    nightStyle: NightStyle;
    error: string | null;
    startTime: number | null;
}

// Initial State
const initialState: SlideshowOperationState = {
    isToggling: false,
    isClearingDisplay: false,
    isSkippingImage: false,
    isShowingImage: false,
    isUpdatingInterval: false,
    isFetchingInterval: false,
    isUpdatingNightMode: false,
    isFetchingNightMode: false,
    displayImagesIntervalMins: 15,
    nightModeEnabled: false,
    nightModeActiveNow: false,
    nightStartHour: 2,
    nightEndHour: 5,
    nightIntervalMins: 60,
    nightStyle: 'hold',
    error: null,
    startTime: null,
};

// Slice
export const slideshowOperationSlice = createSlice({
    name: 'slideshowOperation',
    initialState,
    reducers: {
        setToggleStatus: (state, action: PayloadAction<{
            isToggling: boolean;
        }>) => {
            state.isToggling = action.payload.isToggling;
            state.startTime = action.payload.isToggling ? Date.now() : null;
        },
        setClearDisplayStatus: (state, action: PayloadAction<{
            isClearingDisplay: boolean;
        }>) => {
            state.isClearingDisplay = action.payload.isClearingDisplay;
            state.startTime = action.payload.isClearingDisplay ? Date.now() : null;
        },
        setSkipImageStatus: (state, action: PayloadAction<{
            isSkippingImage: boolean;
        }>) => {
            state.isSkippingImage = action.payload.isSkippingImage;
            state.startTime = action.payload.isSkippingImage ? Date.now() : null;
        },
        setShowImageStatus: (state, action: PayloadAction<{
            isShowingImage: boolean;
        }>) => {
            state.isShowingImage = action.payload.isShowingImage;
            state.startTime = action.payload.isShowingImage ? Date.now() : null;
        },
        setUpdateIntervalStatus: (state, action: PayloadAction<{
            isUpdatingInterval: boolean;
        }>) => {
            state.isUpdatingInterval = action.payload.isUpdatingInterval;
            state.startTime = action.payload.isUpdatingInterval ? Date.now() : null;
        },
        setFetchIntervalStatus: (state, action: PayloadAction<{
            isFetchingInterval: boolean;
        }>) => {
            state.isFetchingInterval = action.payload.isFetchingInterval;
        },
        setDisplayRefreshInterval: (state, action: PayloadAction<number>) => {
            state.displayImagesIntervalMins = action.payload;
        },
        setUpdateNightModeStatus: (state, action: PayloadAction<{
            isUpdatingNightMode: boolean;
        }>) => {
            state.isUpdatingNightMode = action.payload.isUpdatingNightMode;
        },
        setFetchNightModeStatus: (state, action: PayloadAction<{
            isFetchingNightMode: boolean;
        }>) => {
            state.isFetchingNightMode = action.payload.isFetchingNightMode;
        },
        setNightMode: (state, action: PayloadAction<{
            enabled: boolean;
            startHour: number;
            endHour: number;
            intervalMins: number;
            style: NightStyle;
            activeNow?: boolean;
        }>) => {
            state.nightModeEnabled = action.payload.enabled;
            state.nightStartHour = action.payload.startHour;
            state.nightEndHour = action.payload.endHour;
            state.nightIntervalMins = action.payload.intervalMins;
            state.nightStyle = action.payload.style;
            if (action.payload.activeNow !== undefined) {
                state.nightModeActiveNow = action.payload.activeNow;
            }
        },
        setError: (state, action: PayloadAction<string | null>) => {
            state.error = action.payload;
        },
        resetOperation: (state) => {
            state.isToggling = false;
            state.isClearingDisplay = false;
            state.isSkippingImage = false;
            state.isShowingImage = false;
            state.isUpdatingInterval = false;
            state.isFetchingInterval = false;
            state.isUpdatingNightMode = false;
            state.isFetchingNightMode = false;
            state.error = null;
            state.startTime = null;
        }
    }
});

export const fetchDisplayImagesLoopInterval = () => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    // Prevent operation if already fetching
    if (currentState.isFetchingInterval) {
        return;
    }

    try {
        dispatch(slideshowOperationSlice.actions.setFetchIntervalStatus({
            isFetchingInterval: true,
        }));

        // The interval is part of the slideshow status payload on this backend.
        const response = await fetchWithTimeout(getSlideshowStatusUrl());
        const payload: IServerResponse & { data: { interval_seconds: number } } = await response.json();

        if (payload.success && payload.data?.interval_seconds !== undefined) {
            // Convert seconds to minutes for consistency
            const intervalMins = Math.round(payload.data.interval_seconds / 60);
            dispatch(slideshowOperationSlice.actions.setDisplayRefreshInterval(intervalMins));
        } else {
            throw new Error('Failed to fetch display interval');
        }

        dispatch(slideshowOperationSlice.actions.setFetchIntervalStatus({
            isFetchingInterval: false,
        }));

    } catch (error) {
        dispatch(slideshowOperationSlice.actions.setFetchIntervalStatus({
            isFetchingInterval: false,
        }));

        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        dispatch(addAlertSnackbar(
            uuid(),
            'Abrufen des Intervalls der Bilderwiedergabe fehlgeschlagen',
            'error'
        ));
    }
};

export const updateDisplayImagesLoopInterval = (intervalMins: number) => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    // Prevent multiple simultaneous operations
    if (currentState.isToggling || currentState.isClearingDisplay ||
        currentState.isSkippingImage || currentState.isShowingImage || currentState.isUpdatingInterval) {
        return;
    }

    const actionId = uuid();

    try {
        dispatch(slideshowOperationSlice.actions.setUpdateIntervalStatus({
            isUpdatingInterval: true,
        }));

        dispatch(addLoadingSnackbar(
            actionId,
            'Intervall der Bilderwiedergabe wird aktualisiert'
        ));

        // Convert minutes to seconds for the API
        const intervalSeconds = intervalMins * 60;

        const response = await fetchWithTimeout(getSlideshowIntervalUrl(), {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                interval_seconds: intervalSeconds
            })
        });

        const payload: IServerResponse & { interval_seconds: number } = await response.json();

        if (payload.success) {
            dispatch(slideshowOperationSlice.actions.setDisplayRefreshInterval(intervalMins));

            dispatch(addAlertSnackbar(
                uuid(),
                `Intervall der Bilderwiedergabe erfolgreich auf ${intervalMins} Minuten aktualisiert`,
                'success'
            ));

            // Start timer
            dispatch(resetTimer('slideshow-actions-timer'));
            dispatch(startTimer('slideshow-actions-timer'));
        } else {
            throw new Error(payload.message || 'Failed to update display interval');
        }

        dispatch(slideshowOperationSlice.actions.setUpdateIntervalStatus({
            isUpdatingInterval: false,
        }));

        dispatch(removeLoadingSnackbar(actionId));

    } catch (error) {
        dispatch(slideshowOperationSlice.actions.setUpdateIntervalStatus({
            isUpdatingInterval: false,
        }));

        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        dispatch(addAlertSnackbar(
            uuid(),
            'Aktualisierung des Intervalls der Bilderwiedergabes fehlgeschlagen',
            'error'
        ));

        dispatch(removeLoadingSnackbar(actionId));
    }
};

export const fetchNightMode = () => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    if (currentState.isFetchingNightMode) {
        return;
    }

    try {
        dispatch(slideshowOperationSlice.actions.setFetchNightModeStatus({
            isFetchingNightMode: true,
        }));

        // Night mode ships as part of the slideshow status payload.
        const response = await fetchWithTimeout(getSlideshowStatusUrl());
        const payload: IServerResponse & {
            data: {
                night_mode: {
                    enabled: boolean;
                    start_hour: number;
                    end_hour: number;
                    interval_seconds: number;
                    style: NightStyle;
                    active_now: boolean;
                }
            }
        } = await response.json();

        if (payload.success && payload.data?.night_mode) {
            const night = payload.data.night_mode;
            dispatch(slideshowOperationSlice.actions.setNightMode({
                enabled: night.enabled,
                startHour: night.start_hour,
                endHour: night.end_hour,
                intervalMins: Math.round(night.interval_seconds / 60),
                style: night.style,
                activeNow: night.active_now,
            }));
        } else {
            throw new Error('Failed to fetch night mode');
        }

        dispatch(slideshowOperationSlice.actions.setFetchNightModeStatus({
            isFetchingNightMode: false,
        }));

    } catch (error) {
        dispatch(slideshowOperationSlice.actions.setFetchNightModeStatus({
            isFetchingNightMode: false,
        }));

        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        dispatch(addAlertSnackbar(
            uuid(),
            'Abrufen des Nachtmodus fehlgeschlagen',
            'error'
        ));
    }
};

const NIGHT_STYLE_SUFFIX: Record<NightStyle, (intervalMins: number) => string> = {
    slower: (intervalMins) => `${intervalMins} Minuten Intervall`,
    hold: () => 'angehaltenem Bild',
    pause: () => 'ausgeschaltetem Bildschirm',
};

export const updateNightMode = (
    enabled: boolean,
    startHour: number,
    endHour: number,
    intervalMins: number,
    style: NightStyle
) => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    if (getState().slideshowOperation.isUpdatingNightMode) {
        return;
    }

    const actionId = uuid();

    try {
        dispatch(slideshowOperationSlice.actions.setUpdateNightModeStatus({
            isUpdatingNightMode: true,
        }));

        dispatch(addLoadingSnackbar(
            actionId,
            'Nachtmodus wird aktualisiert'
        ));

        const response = await fetchWithTimeout(getSlideshowNightModeUrl(), {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                enabled,
                start_hour: startHour,
                end_hour: endHour,
                interval_seconds: intervalMins * 60,
                style,
            })
        });

        const payload: IServerResponse = await response.json();

        if (payload.success) {
            dispatch(slideshowOperationSlice.actions.setNightMode({
                enabled,
                startHour,
                endHour,
                intervalMins,
                style,
            }));

            dispatch(addAlertSnackbar(
                uuid(),
                !enabled
                    ? 'Nachtmodus deaktiviert'
                    : `Nachtmodus aktiv von ${startHour}:00 bis ${endHour}:00 Uhr mit ${NIGHT_STYLE_SUFFIX[style](intervalMins)}`,
                'success'
            ));
        } else {
            throw new Error(payload.message || 'Failed to update night mode');
        }

        dispatch(slideshowOperationSlice.actions.setUpdateNightModeStatus({
            isUpdatingNightMode: false,
        }));

        dispatch(removeLoadingSnackbar(actionId));

    } catch (error) {
        dispatch(slideshowOperationSlice.actions.setUpdateNightModeStatus({
            isUpdatingNightMode: false,
        }));

        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        dispatch(addAlertSnackbar(
            uuid(),
            'Aktualisierung des Nachtmodus fehlgeschlagen',
            'error'
        ));

        dispatch(removeLoadingSnackbar(actionId));
    }
};

const SKIP_WHILE_REFRESHING = 'Display wird gerade aktualisiert, bitte gleich erneut versuchen';

export const skipImageThunk = () => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    // Prevent multiple simultaneous operations
    if (currentState.isToggling || currentState.isClearingDisplay ||
        currentState.isSkippingImage || currentState.isShowingImage || currentState.isUpdatingInterval) {
        return;
    }

    const actionId = uuid();

    try {
        dispatch(slideshowOperationSlice.actions.setSkipImageStatus({
            isSkippingImage: true,
        }));

        dispatch(addLoadingSnackbar(
            actionId,
            'Aktuelles Bild wird übersprungen'
        ));

        const skipResponse = await fetchWithTimeout(getSkipSlideshowImageUrl(), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
        });

        if (skipResponse.status === 409) {
            throw new Error(SKIP_WHILE_REFRESHING);
        }

        const clearData: IServerResponse = await skipResponse.json();

        if (!clearData.success) {
            throw new Error('Bild überspringen fehlgeschlagen');
        }


        dispatch(addAlertSnackbar(
            uuid(),
            'Aktuelles Bild erfolgreich übersprungen',
            'success'
        ));

        dispatch(resetTimer('slideshow-actions-timer'));
        dispatch(startTimer('slideshow-actions-timer'));

        dispatch(slideshowOperationSlice.actions.setSkipImageStatus({
            isSkippingImage: false,
        }));

        dispatch(removeLoadingSnackbar(actionId));

    } catch (error) {
        dispatch(slideshowOperationSlice.actions.setSkipImageStatus({
            isSkippingImage: false,
        }));

        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        dispatch(addAlertSnackbar(
            uuid(),
            error instanceof Error && error.message === SKIP_WHILE_REFRESHING
                ? SKIP_WHILE_REFRESHING
                : 'Aktuelles Bild überspringen fehlgeschlagen',
            'error'
        ));

        dispatch(removeLoadingSnackbar(actionId));
    }
};

const SHOW_WHILE_PAUSED = 'Die Diashow ist pausiert, starte sie zuerst';

export const showImageThunk = (id: number) => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    if (currentState.isToggling || currentState.isClearingDisplay ||
        currentState.isSkippingImage || currentState.isShowingImage || currentState.isUpdatingInterval) {
        return;
    }

    const actionId = uuid();

    try {
        dispatch(slideshowOperationSlice.actions.setShowImageStatus({ isShowingImage: true }));
        dispatch(addLoadingSnackbar(actionId, 'Bild wird angezeigt'));

        const response = await fetchWithTimeout(getShowSlideshowImageUrl(), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id }),
        });
        const data: IServerResponse = await response.json();

        if (response.status === 409) {
            throw new Error(data.message === 'Slideshow is paused' ? SHOW_WHILE_PAUSED : SKIP_WHILE_REFRESHING);
        }
        if (!data.success) {
            throw new Error('Bild anzeigen fehlgeschlagen');
        }

        dispatch(addAlertSnackbar(uuid(), 'Bild wird auf dem Bilderrahmen angezeigt', 'success'));

        dispatch(resetTimer('slideshow-actions-timer'));
        dispatch(startTimer('slideshow-actions-timer'));
        dispatch(fetchSlideshowImages());
    } catch (error) {
        const message = error instanceof Error ? error.message : 'Unbekannter Fehler';
        dispatch(slideshowOperationSlice.actions.setError(message));
        dispatch(addAlertSnackbar(
            uuid(),
            message === SKIP_WHILE_REFRESHING || message === SHOW_WHILE_PAUSED ? message : 'Bild anzeigen fehlgeschlagen',
            'error'
        ));
    } finally {
        dispatch(slideshowOperationSlice.actions.setShowImageStatus({ isShowingImage: false }));
        dispatch(removeLoadingSnackbar(actionId));
    }
};

export const clearDisplayThunk = () => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    // Prevent multiple simultaneous operations
    if (currentState.isToggling || currentState.isClearingDisplay ||
        currentState.isSkippingImage || currentState.isShowingImage || currentState.isUpdatingInterval) {
        return;
    }

    const actionId = uuid();

    try {
        // Start clear display operation
        dispatch(slideshowOperationSlice.actions.setClearDisplayStatus({
            isClearingDisplay: true,
        }));

        // Add loading snackbar
        dispatch(addLoadingSnackbar(
            actionId,
            'Bildschirm wird geleert'
        ));

        const clearResponse = await fetchWithTimeout(getClearDisplayUrl(), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
        });

        const clearData: IServerResponse = await clearResponse.json();

        if (!clearData.success) {
            throw new Error('Bildschirm leeren fehlgeschlagen');
        }

        // Success notification
        dispatch(addAlertSnackbar(
            uuid(),
            'Bildschirm erfolgreich geleert',
            'success'
        ));

        // Start timer
        dispatch(resetTimer('slideshow-actions-timer'));
        dispatch(startTimer('slideshow-actions-timer'));

        // Reset clear display status
        dispatch(slideshowOperationSlice.actions.setClearDisplayStatus({
            isClearingDisplay: false,
        }));

        dispatch(removeLoadingSnackbar(actionId));

    } catch (error) {
        // Reset clear display status
        dispatch(slideshowOperationSlice.actions.setClearDisplayStatus({
            isClearingDisplay: false,
        }));

        // Set error
        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        // Error notification
        dispatch(addAlertSnackbar(
            uuid(),
            'Fehler beim Leeren des Bildschirms',
            'error'
        ));

        dispatch(removeLoadingSnackbar(actionId));
    }
};

export const toggleSlideshowThunk = () => async (
    dispatch: AppDispatch,
    getState: () => RootState
) => {
    const currentState = getState().slideshowOperation;

    // Prevent multiple simultaneous operations
    if (currentState.isToggling || currentState.isClearingDisplay ||
        currentState.isSkippingImage || currentState.isShowingImage || currentState.isUpdatingInterval) {
        return;
    }

    const actionId = uuid();

    try {
        // Start toggle operation
        dispatch(slideshowOperationSlice.actions.setToggleStatus({
            isToggling: true,
        }));

        // Add loading snackbar
        dispatch(addLoadingSnackbar(
            actionId,
            'Status der Bilderwiedergabe wird gewechselt'
        ));

        // Fetch current slideshow status
        const statusResponse = await fetchWithTimeout(getSlideshowStatusUrl());
        const statusData = await statusResponse.json();

        if (!statusData.success) {
            throw new Error('Aktueller Status konnte nicht abgerufen werden');
        }

        const wasActive = statusData.data.active;
        const action = wasActive ? 'stop' : 'start';

        const toggleResponse = await fetchWithTimeout(getSlideshowUrl(), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action })
        });

        const toggleData: IServerResponse = await toggleResponse.json();

        if (!toggleData.success) {
            throw new Error('Statusänderung fehlgeschlagen');
        }

        const monitorOperation = async () => {
            const startTime = Date.now();

            while (Date.now() - startTime < MAX_OPERATION_WAIT_TIME) {
                try {
                    const statusCheck = await fetchWithTimeout(getSlideshowStatusUrl());
                    const statusCheckData = await statusCheck.json();

                    if (statusCheckData.success &&
                        statusCheckData.data.active !== wasActive) {

                        // Reset toggle status
                        dispatch(slideshowOperationSlice.actions.setToggleStatus({
                            isToggling: false,
                        }));

                        dispatch(addTimer({
                            id: 'slideshow-actions-timer',
                        }));

                        dispatch(resetTimer('slideshow-actions-timer'));
                        dispatch(startTimer('slideshow-actions-timer'));


                        // Clear frame if stopping
                        if (action === 'stop') {
                            await fetchWithTimeout(getClearDisplayUrl(), { method: 'POST' });
                        }

                        dispatch(removeLoadingSnackbar(actionId));

                        // Success notification
                        dispatch(addAlertSnackbar(
                            uuid(),
                            `Bilderwiedergabe erfolgreich ${action === 'stop' ? 'gestoppt' : 'gestartet'}`,
                            'success'
                        ));

                        return;
                    }
                } catch (checkError) {
                    console.error('Status check failed', checkError);
                    dispatch(removeLoadingSnackbar(actionId));
                }

                // Wait before next check
                await new Promise(resolve => setTimeout(resolve, 2000));
            }

            // Timeout occurred
            dispatch(slideshowOperationSlice.actions.setToggleStatus({
                isToggling: false,
            }));

            dispatch(slideshowOperationSlice.actions.setError(
                'Zeitüberschreitung bei Statusänderung'
            ));

            // Error notification
            dispatch(addAlertSnackbar(
                uuid(),
                'Zeitüberschreitung bei Statusänderung',
                'error'
            ));

            dispatch(removeLoadingSnackbar(actionId));
        };

        // Start monitoring in background
        await monitorOperation();

    } catch (error) {
        // Reset toggle status
        dispatch(slideshowOperationSlice.actions.setToggleStatus({
            isToggling: false,
        }));

        // Set error
        dispatch(slideshowOperationSlice.actions.setError(
            error instanceof Error ? error.message : 'Unbekannter Fehler'
        ));

        // Error notification
        dispatch(addAlertSnackbar(
            uuid(),
            'Fehler bei der Statusänderung der Bildwiedergabe',
            'error'
        ));

        dispatch(removeLoadingSnackbar(actionId));
    }
};

// Selectors
export const selectSlideshowOperation = (state: RootState) => state.slideshowOperation;
export const selectDisplayRefreshInterval = (state: RootState) => state.slideshowOperation.displayImagesIntervalMins;
export const selectIsUpdatingInterval = (state: RootState) => state.slideshowOperation.isUpdatingInterval;
export const selectIsFetchingInterval = (state: RootState) => state.slideshowOperation.isFetchingInterval;
export const selectNightModeEnabled = (state: RootState) => state.slideshowOperation.nightModeEnabled;
export const selectNightModeActiveNow = (state: RootState) => state.slideshowOperation.nightModeActiveNow;
export const selectNightStartHour = (state: RootState) => state.slideshowOperation.nightStartHour;
export const selectNightEndHour = (state: RootState) => state.slideshowOperation.nightEndHour;
export const selectNightIntervalMins = (state: RootState) => state.slideshowOperation.nightIntervalMins;
export const selectNightStyle = (state: RootState) => state.slideshowOperation.nightStyle;
export const selectIsUpdatingNightMode = (state: RootState) => state.slideshowOperation.isUpdatingNightMode;
export const selectIsFetchingNightMode = (state: RootState) => state.slideshowOperation.isFetchingNightMode;
export const selectIsAnyOperationActive = (state: RootState) => {
    const ops = state.slideshowOperation;
    return ops.isToggling || ops.isClearingDisplay || ops.isSkippingImage || ops.isUpdatingInterval;
};

export const {
    setToggleStatus,
    setClearDisplayStatus,
    setSkipImageStatus,
    setUpdateIntervalStatus,
    setFetchIntervalStatus,
    setDisplayRefreshInterval,
    setUpdateNightModeStatus,
    setFetchNightModeStatus,
    setNightMode,
    setError,
    resetOperation
} = slideshowOperationSlice.actions;

export default slideshowOperationSlice.reducer;