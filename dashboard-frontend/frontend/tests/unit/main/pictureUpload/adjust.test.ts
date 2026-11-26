import { describe, expect, it } from 'vitest';
import { adjustPixels, isNeutral, NEUTRAL_ADJUSTMENT } from '@/main/pictureUpload/imageAdjust/adjust';

const pixel = (value: number, alpha = 255) => new Uint8ClampedArray([value, value, value, alpha]);

const adjust = (value: number, brightness: number, contrast: number) =>
  adjustPixels(pixel(value), { brightness, contrast })[0];

describe('adjustPixels', () => {
  it('leaves the photo untouched when neutral', () => {
    const rgba = new Uint8ClampedArray([0, 64, 128, 200, 255, 10, 90, 255]);

    expect(adjustPixels(rgba, NEUTRAL_ADJUSTMENT)).toEqual(rgba);
    expect(isNeutral(NEUTRAL_ADJUSTMENT)).toBe(true);
  });

  it('brightens and darkens by up to half the range', () => {
    expect(adjust(128, 100, 0)).toBe(255);
    expect(adjust(128, -100, 0)).toBe(0);
    expect(adjust(100, 50, 0)).toBe(164);
  });

  it('spreads the values away from mid gray with more contrast', () => {
    expect(adjust(100, 0, 50)).toBeLessThan(100);
    expect(adjust(160, 0, 50)).toBeGreaterThan(160);
    expect(adjust(0, 0, 100)).toBe(0);
    expect(adjust(255, 0, 100)).toBe(255);
  });

  it('flattens everything to mid gray at the lowest contrast', () => {
    expect(adjust(0, 0, -100)).toBe(128);
    expect(adjust(255, 0, -100)).toBe(128);
  });

  it('adjusts each channel alike and keeps the alpha', () => {
    const out = adjustPixels(new Uint8ClampedArray([10, 128, 240, 77]), { brightness: 20, contrast: 30 });

    expect(out[3]).toBe(77);
    expect(out[0]).toBeLessThan(out[1]);
    expect(out[1]).toBeLessThan(out[2]);
  });
});
