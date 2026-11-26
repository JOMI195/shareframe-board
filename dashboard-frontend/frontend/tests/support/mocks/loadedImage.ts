// jsdom never fires image load events, so the text step would wait forever for the cropped photo.
export const useLoadedImage = () => ({ image: { width: 800, height: 480 } as HTMLImageElement, error: false });
