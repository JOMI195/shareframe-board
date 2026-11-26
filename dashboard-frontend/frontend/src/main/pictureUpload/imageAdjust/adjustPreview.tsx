import React, { useEffect, useMemo, useRef, useState } from 'react';
import { Box } from '@mui/material';
import { pixelsToBlob, readPixels } from '@/common/utils/canvas/pixels';
import { EPAPER_HEIGHT, EPAPER_WIDTH, ditherToEpaper } from '@/main/pictureUpload/epaper/dither';
import { Adjustment, adjustPixels } from './adjust';

// Large enough for the dialog, small enough to repaint on every slider move.
const PREVIEW_MAX_WIDTH = 1200;

interface AdjustPreviewProps {
    image: HTMLImageElement;
    adjustment: Adjustment;
    epaper: boolean;
    alt: string;
}

// An img, not a canvas: browsers downscale a canvas with cheap filtering, which turns the dither into noise.
const AdjustPreview: React.FC<AdjustPreviewProps> = ({ image, adjustment, epaper, alt }) => {
    const [url, setUrl] = useState<string | null>(null);
    const urlRef = useRef<string | null>(null);

    const base = useMemo(() => {
        if (epaper) return readPixels(image, EPAPER_WIDTH, EPAPER_HEIGHT);
        const width = Math.min(image.width, PREVIEW_MAX_WIDTH);
        return readPixels(image, width, Math.round((image.height * width) / image.width));
    }, [image, epaper]);

    useEffect(() => {
        if (!base) return;
        let active = true;
        // Slider moves within one frame collapse into a single repaint with the latest value.
        const frame = requestAnimationFrame(() => {
            const adjusted = adjustPixels(base.data, adjustment);
            const pixels = { ...base, data: epaper ? ditherToEpaper(adjusted, base.width, base.height) : adjusted };
            pixelsToBlob(pixels, epaper ? 'image/png' : 'image/jpeg', 0.92)
                .then((blob) => {
                    if (!active) return;
                    if (urlRef.current) URL.revokeObjectURL(urlRef.current);
                    urlRef.current = URL.createObjectURL(blob);
                    setUrl(urlRef.current);
                })
                .catch(() => undefined);
        });
        return () => {
            active = false;
            cancelAnimationFrame(frame);
        };
    }, [base, adjustment, epaper]);

    useEffect(() => () => {
        if (urlRef.current) URL.revokeObjectURL(urlRef.current);
    }, []);

    if (!url) return null;
    return (
        <Box
            component="img"
            src={url}
            alt={alt}
            sx={{ display: 'block', width: '100%', height: '100%', objectFit: 'contain' }}
        />
    );
};

export default AdjustPreview;
