import { createSlice, PayloadAction } from '@reduxjs/toolkit';
import { RootState } from '@/store';

export interface LoadingWallState {
    isLoadingWallVisible: boolean;
    message: string;
    hideAfter?: number;
}

const initialState: LoadingWallState = {
    isLoadingWallVisible: false,
    message: "",
    hideAfter: undefined,
};

export const loadingWallSlice = createSlice({
    name: 'loadingWall',
    initialState,
    reducers: {
        loadingWallVisible: (state, action: PayloadAction<{ message: string; maxMs: number }>) => {
            state.isLoadingWallVisible = true;
            state.message = action.payload.message;
            state.hideAfter = Date.now() + action.payload.maxMs;
        },
        loadingWallHidden: (state) => {
            state.isLoadingWallVisible = false;
            state.message = "";
            state.hideAfter = undefined;
        },
    }
});

// The wall goes once the board answers again; maxMs is the latest it gives up waiting.
export const showLoadingWall = (
    message: string,
    maxMs = 3 * 60 * 1000
) => ({
    type: loadingWallVisible.type,
    payload: { message, maxMs },
});

export const hideLoadingWall = () => ({
    type: loadingWallHidden.type,
});

export const {
    loadingWallVisible,
    loadingWallHidden,
} = loadingWallSlice.actions;

export const selectLoadingWallState = (state: RootState) => state.loadingWall;

export default loadingWallSlice.reducer;