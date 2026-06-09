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
        loadingWallVisible: (state, action: PayloadAction<string>) => {
            state.isLoadingWallVisible = true;
            state.message = action.payload;
            state.hideAfter = Date.now() + 3 * 60 * 1000; // 3 minutes in the future
        },
        loadingWallHidden: (state) => {
            if (state.hideAfter && Date.now() >= state.hideAfter) {
                state.isLoadingWallVisible = false;
                state.message = "";
                state.hideAfter = undefined;
            }
        },
    }
});

export const showLoadingWall = (
    message: string
) => ({
    type: loadingWallVisible.type,
    payload: message,
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