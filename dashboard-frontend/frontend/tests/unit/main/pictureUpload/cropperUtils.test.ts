import { describe, expect, it } from 'vitest';
import { containZoom, getCroppedImg, getRadianAngle, rotateSize, stepRotation } from '@/main/pictureUpload/imageCropping/cropper/utils';

describe('getRadianAngle', () => {
  it('converts degrees to radians', () => {
    expect(getRadianAngle(0)).toBe(0);
    expect(getRadianAngle(180)).toBeCloseTo(Math.PI);
    expect(getRadianAngle(-90)).toBeCloseTo(-Math.PI / 2);
  });
});

describe('rotateSize', () => {
  it('leaves an unrotated rectangle unchanged', () => {
    expect(rotateSize(800, 600, 0)).toEqual({ width: 800, height: 600 });
  });

  it('swaps the sides at 90 degrees', () => {
    const { width, height } = rotateSize(800, 600, 90);
    expect(width).toBeCloseTo(600);
    expect(height).toBeCloseTo(800);
  });

  it('grows the bounding box at 45 degrees', () => {
    const { width, height } = rotateSize(100, 100, 45);
    expect(width).toBeCloseTo(Math.sqrt(2) * 100);
    expect(height).toBeCloseTo(Math.sqrt(2) * 100);
  });

  it('is sign-agnostic', () => {
    expect(rotateSize(800, 600, -90)).toEqual(rotateSize(800, 600, 90));
  });
});

describe('containZoom', () => {
  const crop = { width: 500, height: 300 };

  it('fits the height of a photo taller than the frame', () => {
    expect(containZoom({ width: 500, height: 375 }, crop, 0, 0.1, 4)).toBeCloseTo(0.8);
  });

  it('fits the width of a panorama wider than the frame', () => {
    expect(containZoom({ width: 1000, height: 300 }, crop, 0, 0.1, 4)).toBeCloseTo(0.5);
  });

  it('uses the rotated sides', () => {
    expect(containZoom({ width: 500, height: 300 }, crop, 90, 0.1, 4)).toBeCloseTo(0.6);
  });

  it('stays within the zoom range', () => {
    expect(containZoom({ width: 500, height: 30000 }, crop, 0, 0.1, 4)).toBe(0.1);
    expect(containZoom({ width: 50, height: 30 }, crop, 0, 0.1, 4)).toBe(4);
  });
});

describe('getCroppedImg', () => {
  it('rejects a sub-pixel crop before touching the canvas', async () => {
    const file = new File(['x'], 'photo.jpg', { type: 'image/jpeg' });
    await expect(getCroppedImg(file, { width: 0.5, height: 10, x: 0, y: 0 })).rejects.toThrow(
      'Der Zuschnitt ist noch nicht bereit. Bitte erneut versuchen.',
    );
  });

  it('rejects a crop measured before the cropper had a size', async () => {
    const file = new File(['x'], 'photo.jpg', { type: 'image/jpeg' });
    await expect(getCroppedImg(file, { width: NaN, height: NaN, x: 0, y: 0 })).rejects.toThrow(
      'Der Zuschnitt ist noch nicht bereit. Bitte erneut versuchen.',
    );
  });
});

describe('stepRotation', () => {
  it.each([
    ['turns right by one step', 0, 1, 22.5],
    ['keeps stepping', 22.5, 1, 45],
    ['goes right to the next step from between two', 10, 1, 22.5],
    ['goes left to the previous step from between two', 10, -1, 0],
    ['wraps past a full turn', 337.5, 1, 0],
    ['wraps below zero', 0, -1, 337.5],
  ] as const)('%s', (_name, rotation, direction, expected) => {
    expect(stepRotation(rotation, 22.5, direction)).toBe(expected);
  });
});
