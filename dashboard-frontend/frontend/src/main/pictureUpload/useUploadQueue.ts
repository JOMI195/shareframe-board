import { useCallback, useEffect, useRef, useState } from 'react';
import uuid from 'react-uuid';
import { useAppDispatch } from '@/store';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';

export interface ImageStatus {
    id: string;
    file: File;
    status: 'pending' | 'cropping' | 'uploaded';
    croppedFile?: File;
}

const getFileKey = (file: File) => `${file.name}-${file.size}-${file.lastModified}`;

export const useUploadQueue = (maxFiles: number) => {
    const dispatch = useAppDispatch();
    const [imageStatuses, setImageStatuses] = useState<ImageStatus[]>([]);
    const [imagePreviews, setImagePreviews] = useState<{ [id: string]: string }>({});
    const [previewErrors, setPreviewErrors] = useState<{ [id: string]: string }>({});

    const addImages = (files: File[]) => {
        const knownKeys = new Set(imageStatuses.map(status => getFileKey(status.file)));
        const additions: ImageStatus[] = [];
        let skipped = 0;

        files.forEach(file => {
            const key = getFileKey(file);
            if (knownKeys.has(key) || imageStatuses.length + additions.length >= maxFiles) {
                skipped++;
                return;
            }
            knownKeys.add(key);
            additions.push({
                id: `${key}-${Date.now()}-${Math.random()}`,
                file,
                status: 'pending'
            });
        });

        if (skipped > 0) {
            dispatch(addAlertSnackbar(uuid(), `${skipped === 1 ? 'Ein Foto wurde' : `${skipped} Fotos wurden`} nicht hinzugefügt, weil sie schon ausgewählt sind oder das Maximum von ${maxFiles} erreicht ist.`, 'warning'));
        }

        if (additions.length > 0) {
            setImageStatuses(prev => [...prev, ...additions]);
        }
    };

    const reset = () => {
        setImageStatuses([]);
        setImagePreviews({});
        setPreviewErrors({});
    };

    const activeUrlRefs = useRef<{ [id: string]: string }>({});

    useEffect(() => {
        const newPreviewsMap: { [id: string]: string } = {};
        const currentImageIdsInStatus = new Set(imageStatuses.map(status => status.id));

        const urlsToRevoke: string[] = [];
        for (const imageId in activeUrlRefs.current) {
            if (!currentImageIdsInStatus.has(imageId)) {
                urlsToRevoke.push(activeUrlRefs.current[imageId]);
            }
        }

        imageStatuses.forEach(imageStatus => {
            newPreviewsMap[imageStatus.id] = activeUrlRefs.current[imageStatus.id] ?? URL.createObjectURL(imageStatus.file);
        });

        setImagePreviews(newPreviewsMap);
        setPreviewErrors(prev => {
            const staleIds = Object.keys(prev).filter(id => !currentImageIdsInStatus.has(id));
            if (staleIds.length === 0) {
                return prev;
            }
            const next = { ...prev };
            staleIds.forEach(id => delete next[id]);
            return next;
        });

        activeUrlRefs.current = newPreviewsMap;

        // No cleanup revoking all URLs: it would run before every re-run and leave
        // reused previews pointing at revoked blobs. Unmount is handled below.
        urlsToRevoke.forEach(url => URL.revokeObjectURL(url));
    }, [imageStatuses]);

    useEffect(() => {
        return () => {
            Object.values(activeUrlRefs.current).forEach(url => URL.revokeObjectURL(url));
            activeUrlRefs.current = {};
        };
    }, []);

    const markPreviewBroken = useCallback((id: string, reason: string) => {
        setPreviewErrors(prev => (prev[id] ? prev : { ...prev, [id]: reason }));
    }, []);

    return { imageStatuses, setImageStatuses, addImages, reset, imagePreviews, previewErrors, markPreviewBroken };
};
