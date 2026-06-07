import { createSlice, PayloadAction, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '..';

// Constants
const DEFAULT_DURATION = 180; // 3 minutes in seconds

// Types
export interface Timer {
    id: string;
    duration: number;      // Original duration in seconds
    remaining: number;     // Current remaining time in seconds
    isActive: boolean;     // Whether timer is currently running
    startTimestamp?: number; // When the timer was last started (Unix timestamp)
}

interface TimerState {
    timers: Record<string, Timer>;
    lastSyncTime: number;  // Track when we last synced with actual time
}

// Initial state
const initialState: TimerState = {
    timers: {},
    lastSyncTime: Date.now()
};

// Thunks
export const syncTimers = createAsyncThunk(
    'timers/sync',
    async (_, { getState, dispatch }) => {
        const state = getState() as RootState;
        const { timers, lastSyncTime } = state.timers;
        const currentTime = Date.now();
        const elapsedMs = currentTime - lastSyncTime;

        // Update each active timer
        Object.entries(timers).forEach(([id, timer]) => {
            if (timer.isActive && timer.startTimestamp) {
                const elapsedSeconds = Math.floor(elapsedMs / 1000);
                const newRemaining = Math.max(0, timer.remaining - elapsedSeconds);

                // Update the timer
                dispatch(updateTimer({ id, remaining: newRemaining }));

                // Auto-stop if timer reached zero
                if (newRemaining === 0) {
                    dispatch(stopTimer(id));
                }
            }
        });

        return currentTime; // Return current time for lastSyncTime update
    }
);

export const syncSpecificTimer = createAsyncThunk(
    'timers/syncSpecific',
    async (timerId: string, { getState, dispatch }) => {
        const state = getState() as RootState;
        const timer = state.timers.timers[timerId];

        if (timer && timer.isActive && timer.startTimestamp) {
            const currentTime = Date.now();
            const elapsedMs = currentTime - timer.startTimestamp;
            const newRemaining = Math.max(0, timer.duration - Math.floor(elapsedMs / 1000));

            dispatch(updateTimer({
                id: timerId,
                remaining: newRemaining
            }));

            if (newRemaining === 0) {
                dispatch(stopTimer(timerId));
            }

            return { id: timerId, remaining: newRemaining };
        }

        return { id: timerId };
    }
);

// Slice
const timerSlice = createSlice({
    name: 'timers',
    initialState,
    reducers: {
        // Create a new timer
        addTimer(state, action: PayloadAction<{ id: string; duration?: number }>) {
            const { id, duration = DEFAULT_DURATION } = action.payload;
            // Don't overwrite existing timer
            if (!state.timers[id]) {
                state.timers[id] = {
                    id,
                    duration,
                    remaining: duration,
                    isActive: false
                };
            }
        },

        // Start a timer
        startTimer(state, action: PayloadAction<string>) {
            const timer = state.timers[action.payload];
            if (timer) {
                // Ensure we're in sync before starting
                state.lastSyncTime = Date.now();
                timer.isActive = true;
                timer.startTimestamp = Date.now();
            }
        },

        // Pause a timer
        stopTimer(state, action: PayloadAction<string>) {
            const timer = state.timers[action.payload];
            if (timer) {
                timer.isActive = false;
                timer.startTimestamp = undefined;
            }
        },

        // Reset a timer to its original duration
        resetTimer(state, action: PayloadAction<string>) {
            const timer = state.timers[action.payload];
            if (timer) {
                timer.remaining = timer.duration;
                timer.isActive = false;
                timer.startTimestamp = undefined;
            }
        },

        // Update timer properties (primarily for remaining time)
        updateTimer(state, action: PayloadAction<{ id: string; remaining?: number }>) {
            const { id, remaining } = action.payload;
            const timer = state.timers[id];

            if (timer && remaining !== undefined) {
                timer.remaining = remaining;

                // Auto-stop if timer reached zero
                if (remaining <= 0) {
                    timer.remaining = 0;
                    timer.isActive = false;
                    timer.startTimestamp = undefined;
                }
            }
        },

        // Remove a timer
        removeTimer(state, action: PayloadAction<string>) {
            delete state.timers[action.payload];
        },

        // Reset all timers
        resetAllTimers(state) {
            Object.values(state.timers).forEach(timer => {
                timer.remaining = timer.duration;
                timer.isActive = false;
                timer.startTimestamp = undefined;
            });
        }
    },
    extraReducers: (builder) => {
        builder.addCase(syncTimers.fulfilled, (state, action) => {
            state.lastSyncTime = action.payload;
        });
    }
});

// Selectors
export const selectTimer = (state: RootState, id: string): Timer | undefined =>
    state.timers.timers[id];

export const selectAllTimers = (state: RootState): Record<string, Timer> =>
    state.timers.timers;

export const selectActiveTimers = (state: RootState): Timer[] =>
    Object.values(state.timers.timers).filter((timer): timer is Timer =>
        timer !== undefined && timer.isActive);

// Format time helper (mm:ss)
export const formatTime = (seconds: number): string => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
};

// Export actions
export const {
    addTimer,
    startTimer,
    stopTimer,
    resetTimer,
    updateTimer,
    removeTimer,
    resetAllTimers
} = timerSlice.actions;

export default timerSlice.reducer;