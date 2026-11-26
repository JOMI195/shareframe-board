import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import uuid from 'react-uuid';
import { boardErrorMessage, unreachableMessage } from '@/common/utils/errors/boardErrors';
import { RootState } from '@/store';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { fetchWithTimeout, SLOW_TIMEOUT_MS } from '@/common/utils/fetch';
import { getDefaultImagesUrl, getRemoveDefaultImageUrl } from '@/assets/endpoints/api/frame';
import { IDefaultImage, IServerResponse } from '@/types';

interface DefaultImagesPayload {
    min: number;
    max: number;
    images: { name: string; width: number; height: number; size_bytes: number }[];
}

interface DefaultImagesList {
    min: number;
    max: number;
    images: IDefaultImage[];
}

export interface DefaultImagesState extends DefaultImagesList {
    isLoading: boolean;
    isChanging: boolean;
    error: string | null;
    lastFetchedAt: number | null;
}

const initialState: DefaultImagesState = {
    images: [],
    min: 3,
    max: 10,
    isLoading: false,
    isChanging: false,
    error: null,
    lastFetchedAt: null,
};

const toList = (data: DefaultImagesPayload): DefaultImagesList => ({
    min: data.min,
    max: data.max,
    images: data.images.map((img) => ({
        name: img.name,
        width: img.width,
        height: img.height,
        sizeBytes: img.size_bytes,
    })),
});

type Answer = IServerResponse & { data: DefaultImagesPayload | null };

type Dispatch = (action: unknown) => unknown;

// Every change answers with the new list, so the page never needs a second round trip.
const change = async (
    dispatch: Dispatch,
    request: () => Promise<Response>,
    texts: { loading: string; success: string; failure: string },
): Promise<{ list: DefaultImagesList } | { error: string }> => {
    const loadingId = uuid();
    dispatch(addLoadingSnackbar(loadingId, texts.loading));
    try {
        const response = await request();
        const payload: Answer = await response.json();
        if (payload.success && payload.data) {
            dispatch(addAlertSnackbar(uuid(), texts.success, 'success'));
            return { list: toList(payload.data) };
        }
        const error = boardErrorMessage(texts.failure, response, payload.message);
        dispatch(addAlertSnackbar(uuid(), error, 'error'));
        return { error };
    } catch {
        const error = unreachableMessage();
        dispatch(addAlertSnackbar(uuid(), error, 'error'));
        return { error };
    } finally {
        dispatch(removeLoadingSnackbar(loadingId));
    }
};

export const fetchDefaultImages = createAsyncThunk(
    'defaultImages/fetch',
    async (_, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getDefaultImagesUrl());
            const payload: Answer = await response.json();
            if (!payload.success || !payload.data) {
                return rejectWithValue(payload.message || 'Standardbilder konnten nicht geladen werden');
            }
            return toList(payload.data);
        } catch (error) {
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const uploadDefaultImage = createAsyncThunk(
    'defaultImages/upload',
    async (file: File, { dispatch, rejectWithValue }) => {
        const result = await change(dispatch, () => fetchWithTimeout(getDefaultImagesUrl(), {
            method: 'POST',
            headers: { 'Content-Type': file.type || 'image/jpeg' },
            body: file,
        }, SLOW_TIMEOUT_MS), {
            loading: 'Standardbild wird hochgeladen',
            success: 'Standardbild hinzugefügt',
            failure: 'Standardbild konnte nicht hochgeladen werden',
        });
        return 'list' in result ? result.list : rejectWithValue(result.error);
    }
);

export const removeDefaultImage = createAsyncThunk(
    'defaultImages/remove',
    async (name: string, { dispatch, rejectWithValue }) => {
        const result = await change(dispatch, () => fetchWithTimeout(getRemoveDefaultImageUrl(), {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name }),
        }), {
            loading: 'Standardbild wird entfernt',
            success: 'Standardbild entfernt',
            failure: 'Standardbild konnte nicht entfernt werden',
        });
        return 'list' in result ? result.list : rejectWithValue(result.error);
    }
);

const applyList = (state: DefaultImagesState, list: DefaultImagesList) => {
    state.images = list.images;
    state.min = list.min;
    state.max = list.max;
    state.error = null;
    state.lastFetchedAt = Date.now();
};

export const defaultImagesSlice = createSlice({
    name: 'defaultImages',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder
            .addCase(fetchDefaultImages.pending, (state) => { state.isLoading = true; })
            .addCase(fetchDefaultImages.fulfilled, (state, action) => {
                state.isLoading = false;
                applyList(state, action.payload);
            })
            .addCase(fetchDefaultImages.rejected, (state, action) => {
                state.isLoading = false;
                state.error = action.payload as string;
            });
        for (const thunk of [uploadDefaultImage, removeDefaultImage]) {
            builder
                .addCase(thunk.pending, (state) => { state.isChanging = true; })
                .addCase(thunk.fulfilled, (state, action) => {
                    state.isChanging = false;
                    applyList(state, action.payload);
                })
                .addCase(thunk.rejected, (state) => { state.isChanging = false; });
        }
    }
});

export const selectDefaultImages = (state: RootState) => state.defaultImages;

export default defaultImagesSlice.reducer;
