import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import uuid from 'react-uuid';
import { RootState } from '@/store';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { getSlideshowImagesUrl, getSlideshowOrderUrl } from '@/assets/endpoints/api/frame';
import { ISlideshowImage, IServerResponse, PlaybackMode } from '@/types';

interface SlideshowImagePayload {
    id: number;
    sender: string;
    received_at: number;
    expires_at: number;
    width: number;
    height: number;
    size_bytes: number;
    position: number;
}

interface SlideshowImagesPayload {
    mode: PlaybackMode;
    custom_order: number[];
    current_image_id: number;
    images: SlideshowImagePayload[];
}

export interface SlideshowImagesState {
    images: ISlideshowImage[];
    mode: PlaybackMode | null;
    currentImageId: number | null;
    isLoading: boolean;
    isUpdatingOrder: boolean;
    error: string | null;
    lastFetchedAt: number | null;
}

const initialState: SlideshowImagesState = {
    images: [],
    mode: null,
    currentImageId: null,
    isLoading: false,
    isUpdatingOrder: false,
    error: null,
    lastFetchedAt: null,
};

const MODE_LABELS: Record<PlaybackMode, string> = {
    shuffle: 'Zufällige Reihenfolge',
    chronological: 'Chronologische Reihenfolge',
    newest_first: 'Neueste zuerst',
    custom: 'Eigene Reihenfolge',
};

export const fetchSlideshowImages = createAsyncThunk(
    'slideshowImages/fetch',
    async (_, { rejectWithValue }) => {
        try {
            const response = await fetchWithTimeout(getSlideshowImagesUrl());
            const payload: IServerResponse & { data: SlideshowImagesPayload } = await response.json();
            if (!payload.success || !payload.data) {
                return rejectWithValue(payload.message || 'Abrufen der Bilder fehlgeschlagen');
            }
            return {
                mode: payload.data.mode,
                currentImageId: payload.data.current_image_id > 0 ? payload.data.current_image_id : null,
                images: payload.data.images.map((img): ISlideshowImage => ({
                    id: img.id,
                    sender: img.sender,
                    receivedAt: img.received_at > 0 ? img.received_at : null,
                    expiresAt: img.expires_at,
                    width: img.width,
                    height: img.height,
                    sizeBytes: img.size_bytes,
                    position: img.position,
                })),
            };
        } catch (error) {
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        }
    }
);

export const updatePlaybackOrder = createAsyncThunk(
    'slideshowImages/updateOrder',
    async ({ mode, customOrder }: { mode: PlaybackMode; customOrder?: number[] }, { dispatch, rejectWithValue }) => {
        const loadingId = uuid();
        try {
            dispatch(addLoadingSnackbar(loadingId, 'Reihenfolge wird geändert'));
            const response = await fetchWithTimeout(getSlideshowOrderUrl(), {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(customOrder ? { mode, custom_order: customOrder } : { mode }),
            });
            const payload: IServerResponse = await response.json();
            if (payload.success) {
                dispatch(addAlertSnackbar(uuid(), `${MODE_LABELS[mode]} ab dem nächsten Bild`, 'success'));
                dispatch(fetchSlideshowImages());
                return mode;
            }
            dispatch(addAlertSnackbar(uuid(), payload.message || 'Ändern der Reihenfolge fehlgeschlagen', 'error'));
            return rejectWithValue(payload.message || 'update failed');
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), 'Ändern der Reihenfolge fehlgeschlagen', 'error'));
            return rejectWithValue(error instanceof Error ? error.message : 'Unknown error');
        } finally {
            dispatch(removeLoadingSnackbar(loadingId));
        }
    }
);

export const slideshowImagesSlice = createSlice({
    name: 'slideshowImages',
    initialState,
    reducers: {},
    extraReducers: (builder) => {
        builder
            .addCase(fetchSlideshowImages.pending, (state) => { state.isLoading = true; })
            .addCase(fetchSlideshowImages.fulfilled, (state, action) => {
                state.isLoading = false;
                state.error = null;
                state.mode = action.payload.mode;
                state.currentImageId = action.payload.currentImageId;
                state.images = action.payload.images;
                state.lastFetchedAt = Date.now();
            })
            .addCase(fetchSlideshowImages.rejected, (state, action) => {
                state.isLoading = false;
                state.error = action.payload as string;
            })
            .addCase(updatePlaybackOrder.pending, (state) => { state.isUpdatingOrder = true; })
            .addCase(updatePlaybackOrder.fulfilled, (state, action) => {
                state.isUpdatingOrder = false;
                state.mode = action.payload;
            })
            .addCase(updatePlaybackOrder.rejected, (state) => { state.isUpdatingOrder = false; });
    }
});

export const selectSlideshowImages = (state: RootState) => state.slideshowImages;

export default slideshowImagesSlice.reducer;
