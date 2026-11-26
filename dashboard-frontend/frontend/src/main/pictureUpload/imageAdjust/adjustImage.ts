import { blobToPixels, pixelsToBlob } from '@/common/utils/canvas/pixels';
import { Adjustment, adjustPixels } from './adjust';

export const adjustImage = async (image: Blob, adjustment: Adjustment): Promise<Blob> => {
    const pixels = await blobToPixels(image);
    return pixelsToBlob({ ...pixels, data: adjustPixels(pixels.data, adjustment) }, 'image/jpeg', 0.95);
};
