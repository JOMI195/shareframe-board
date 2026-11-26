export interface Pixels {
    data: Uint8ClampedArray;
    width: number;
    height: number;
}

const createCanvas = (width: number, height: number) => {
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    return canvas;
};

// Draws the source stretched to the given size; null where the browser has no 2D canvas.
export const readPixels = (source: CanvasImageSource, width: number, height: number): Pixels | null => {
    const context = createCanvas(width, height).getContext('2d', { willReadFrequently: true });
    if (!context) return null;
    context.imageSmoothingEnabled = true;
    context.imageSmoothingQuality = 'high';
    context.drawImage(source, 0, 0, width, height);
    return { data: context.getImageData(0, 0, width, height).data, width, height };
};

const writePixels = (canvas: HTMLCanvasElement, { data, width, height }: Pixels) => {
    canvas.width = width;
    canvas.height = height;
    const context = canvas.getContext('2d');
    if (!context) return;
    const image = context.createImageData(width, height);
    image.data.set(data);
    context.putImageData(image, 0, 0);
};

export const pixelsToBlob = (pixels: Pixels, type: string, quality?: number): Promise<Blob> => {
    const canvas = createCanvas(pixels.width, pixels.height);
    writePixels(canvas, pixels);
    return new Promise((resolve, reject) => {
        canvas.toBlob((blob) => (blob ? resolve(blob) : reject(new Error('Canvas export failed'))), type, quality);
    });
};

export const blobToPixels = async (blob: Blob, width?: number, height?: number): Promise<Pixels> => {
    const bitmap = await createImageBitmap(blob);
    try {
        const pixels = readPixels(bitmap, width ?? bitmap.width, height ?? bitmap.height);
        if (!pixels) throw new Error('Das Foto ist zu groß zum Bearbeiten.');
        return pixels;
    } finally {
        bitmap.close();
    }
};
