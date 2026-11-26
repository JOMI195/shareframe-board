import { useEffect, useState } from 'react';
import { renderEpaper } from './renderEpaper';

// Object URL of the dithered source, rendered on first enable and kept until the source changes.
export const useEpaperPreview = (source: Blob | null, enabled: boolean): string | null => {
    const [result, setResult] = useState<{ source: Blob; url: string } | null>(null);
    const ready = result !== null && result.source === source;

    useEffect(() => {
        if (!enabled || !source || ready) return;
        let active = true;
        renderEpaper(source)
            .then((blob) => {
                if (active) setResult({ source, url: URL.createObjectURL(blob) });
            })
            .catch(() => undefined);
        return () => {
            active = false;
        };
    }, [enabled, source, ready]);

    useEffect(() => () => {
        if (result) URL.revokeObjectURL(result.url);
    }, [result]);

    return enabled && ready ? result.url : null;
};
