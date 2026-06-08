import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { AppDispatch, RootState } from '..';
import { fetchWithTimeout } from '@/common/utils/fetch';


// Types for Slideshow Status State
interface SlideshowStatusState {
    isActive: boolean;
    isLoading: boolean;
    error: string | null;
    lastCheckedAt: number | null;
}

// Initial State
const initialState: SlideshowStatusState = {
    isActive: false,
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

            return payload.data.active;
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
                state.isActive = action.payload;
                state.isLoading = false;
                state.error = null;
                state.lastCheckedAt = Date.now();
            })
            .addCase(checkSlideshowStatusThunk.rejected, (state, action) => {
                state.isLoading = false;
                state.error = action.payload as string;
                state.isActive = false;
            });
    }
});

// Selectors
export const selectSlideshowStatus = (state: RootState) => state.slideshowStatus;

export const {
    resetStatus
} = slideshowStatusSlice.actions;

export default slideshowStatusSlice.reducer;