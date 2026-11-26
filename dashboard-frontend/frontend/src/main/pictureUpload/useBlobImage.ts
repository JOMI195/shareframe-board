import { useCallback, useEffect, useRef, useState } from 'react';

export interface BlobImage {
    blob: Blob;
    url: string;
}

// Owns the object URL of the current blob and revokes it once the blob is replaced or the dialog unmounts.
export const useBlobImage = () => {
    const [image, setImage] = useState<BlobImage | null>(null);
    const urlRef = useRef<string | null>(null);

    const replace = useCallback((blob: Blob | null) => {
        if (urlRef.current) {
            URL.revokeObjectURL(urlRef.current);
        }
        const next = blob ? { blob, url: URL.createObjectURL(blob) } : null;
        urlRef.current = next?.url ?? null;
        setImage(next);
    }, []);

    useEffect(() => () => {
        if (urlRef.current) {
            URL.revokeObjectURL(urlRef.current);
        }
    }, []);

    return [image, replace] as const;
};
