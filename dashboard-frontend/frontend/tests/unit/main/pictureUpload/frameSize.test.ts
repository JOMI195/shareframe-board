import { describe, expect, it } from 'vitest';
import { CANVAS_MARGIN, frameSize } from '@/main/pictureUpload/frameSize';
import { FRAME_ASPECT_RATIO } from '@/main/pictureUpload/frameAspectBox';

describe('frameSize', () => {
  it('fills the width of a tall canvas, minus the margin', () => {
    const { width, height } = frameSize(400, 600);

    expect(width).toBe(400 - 2 * CANVAS_MARGIN);
    expect(height).toBeCloseTo(width / FRAME_ASPECT_RATIO);
  });

  it('fills the height of a wide canvas, minus the margin', () => {
    expect(frameSize(2000, 500).height).toBeCloseTo(500 - 2 * CANVAS_MARGIN);
  });

  it('collapses on a canvas smaller than the margin', () => {
    expect(frameSize(20, 20)).toEqual({ width: 0, height: 0 });
  });
});
