import { blobToPixels, pixelsToBlob } from '@/common/utils/canvas/pixels';
import { EPAPER_HEIGHT, EPAPER_WIDTH, ditherToEpaper } from './dither';

// Stretched, not fitted: the board resizes to the panel without keeping the aspect ratio.
export const renderEpaper = async (image: Blob): Promise<Blob> => {
    const pixels = await blobToPixels(image, EPAPER_WIDTH, EPAPER_HEIGHT);
    return pixelsToBlob({ ...pixels, data: ditherToEpaper(pixels.data, EPAPER_WIDTH, EPAPER_HEIGHT) }, 'image/png');
};
