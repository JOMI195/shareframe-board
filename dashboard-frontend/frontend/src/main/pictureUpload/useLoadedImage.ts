import { useEffect, useState } from 'react';

type LoadedImage = { image: HTMLImageElement | null; error: boolean };

export const useLoadedImage = (src: string): LoadedImage => {
    const [state, setState] = useState<LoadedImage & { src: string }>({ src, image: null, error: false });

    useEffect(() => {
        const image = new Image();
        image.onload = () => setState({ src, image, error: false });
        image.onerror = () => setState({ src, image: null, error: true });
        image.src = src;
        return () => {
            image.onload = null;
            image.onerror = null;
        };
    }, [src]);

    return state.src === src ? state : { image: null, error: false };
};
