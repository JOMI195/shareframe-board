import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { AppDispatch, RootState } from '..';
import { fetchWithTimeout } from '@/common/utils/fetch';


// Types for Slideshow Status State
interface SlideshowStatusState {
    isActive: boolean;
    loopStarted: boolean;
    imageCount: number | null;
    secondsUntilNext: number | null; // remaining until next image; null = unknown/paused
    isLoading: boolean;
    error: string | null;
    lastCheckedAt: number | null;
}

// Initial State
const initialState: SlideshowStatusState = {
    isActive: false,
    loopStarted: false,
    imageCount: null,
    secondsUntilNext: null,
    isLoading: false,
    error: null,
    lastCheckedAt: null,
};

// Async Thunk for checking slideshow status
export const checkSlideshowStatusThunk = createAsyncThunk(
    'slideshowStatus/checkStatus',
    async (_, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout('/api/frame/slideshow/status');
            const payload = await response.json();

            if (!payload.success) {
                return rejectWithValue('Failed to fetch slideshow status');
            }

            const raw = payload.data.seconds_until_next;
            const rawCount = payload.data.image_count;
            return {
                active: payload.data.active as boolean,
                loopStarted: payload.data.loop_started as boolean ?? false,
                imageCount: (typeof rawCount === 'number' && rawCount >= 0) ? rawCount : null,
                secondsUntilNext: (typeof raw === 'number' && raw >= 0) ? raw : null,
            };
        } catch (error) {
            return rejectWithValue(
                error instanceof Error ? error.message : 'Unknown error occurred'
            );
        }
    }
);

// Continuous status checking thunk
export const startContinuousStatusCheck = () => (dispatch: AppDispatch) => {
    let intervalId: NodeJS.Timeout;

    const performCheck = () => {
        dispatch(checkSlideshowStatusThunk());
    };

    // Initial immediate check
    performCheck();

    // Set up continuous interval
    intervalId = setInterval(performCheck, 5000); // Check every 5 seconds

    // Return a function to stop the interval
    return () => {
        if (intervalId) {
            clearInterval(intervalId);
        }
    };
};

// Slice
export const slideshowStatusSlice = createSlice({
    name: 'slideshowStatus',
    initialState,
    reducers: {
        resetStatus: (state) => {
            state.isActive = false;
            state.loopStarted = false;
            state.imageCount = null;
            state.secondsUntilNext = null;
            state.isLoading = false;
            state.error = null;
            state.lastCheckedAt = null;
        }
    },
    extraReducers: (builder) => {
        builder
            .addCase(checkSlideshowStatusThunk.pending, (state) => {
                state.isLoading = true;
            })
            .addCase(checkSlideshowStatusThunk.fulfilled, (state, action) => {
                state.isActive = action.payload.active;
                state.loopStarted = action.payload.loopStarted;
                state.imageCount = action.payload.imageCount;
                state.secondsUntilNext = action.payload.secondsUntilNext;
                state.isLoading = false;
                state.error = null;
                state.lastCheckedAt = Date.now();
            })
            .addCase(checkSlideshowStatusThunk.rejected, (state, action) => {
                state.isLoading = false;
                state.error = action.payload as string;
                state.isActive = false;
                state.loopStarted = false;
                state.imageCount = null;
                state.secondsUntilNext = null;
            });
    }
});

// Selectors
export const selectSlideshowStatus = (state: RootState) => state.slideshowStatus;

export const {
    resetStatus
} = slideshowStatusSlice.actions;

export default slideshowStatusSlice.reducer;