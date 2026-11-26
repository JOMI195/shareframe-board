import { FRAME_ASPECT_RATIO } from './frameAspectBox';

// Room around the frame, so the cropper can show what lies outside the crop.
export const CANVAS_MARGIN = 16;

// Every tool draws the frame at this size, so the photo does not jump when switching tools.
export const frameSize = (canvasWidth: number, canvasHeight: number) => {
    const width = Math.max(0, Math.min(canvasWidth - 2 * CANVAS_MARGIN, (canvasHeight - 2 * CANVAS_MARGIN) * FRAME_ASPECT_RATIO));
    return { width, height: width / FRAME_ASPECT_RATIO };
};
