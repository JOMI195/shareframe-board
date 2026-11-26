import { describe, expect, it } from 'vitest';
import { EPAPER_HEIGHT, EPAPER_WIDTH, applyToneCurve, ditherSerpentine, ditherToEpaper } from '@/main/pictureUpload/epaper/dither';

const grayImage = (values: number[]) => {
  const rgba = new Uint8ClampedArray(values.length * 4);
  values.forEach((value, i) => rgba.set([value, value, value, 255], i * 4));
  return rgba;
};

const uniform = (value: number) => grayImage(new Array(EPAPER_WIDTH * EPAPER_HEIGHT).fill(value));

const luma = (rgba: Uint8ClampedArray) => Array.from({ length: rgba.length / 4 }, (_, i) => rgba[i * 4]);

describe('ditherToEpaper', () => {
  it('keeps white white', () => {
    expect(luma(ditherToEpaper(uniform(255), EPAPER_WIDTH, EPAPER_HEIGHT)).every((v) => v === 255)).toBe(true);
  });

  it('keeps black black', () => {
    expect(luma(ditherToEpaper(uniform(0), EPAPER_WIDTH, EPAPER_HEIGHT)).every((v) => v === 0)).toBe(true);
  });

  it('outputs only opaque black or white pixels', () => {
    const values = Array.from({ length: 64 * 32 }, (_, i) => (i * 37) % 256);
    const out = ditherToEpaper(grayImage(values), 64, 32);

    for (let i = 0; i < out.length; i += 4) {
      expect([0, 255]).toContain(out[i]);
      expect(out[i + 1]).toBe(out[i]);
      expect(out[i + 2]).toBe(out[i]);
      expect(out[i + 3]).toBe(255);
    }
  });

  // A flat image skips the stretch, so only the midtone lift applies.
  it('turns flat mid gray into slightly less than half black', () => {
    const black = luma(ditherToEpaper(uniform(128), EPAPER_WIDTH, EPAPER_HEIGHT)).filter((v) => v === 0).length;

    const share = black / (EPAPER_WIDTH * EPAPER_HEIGHT);
    expect(share).toBeCloseTo(1 - Math.pow(128 / 255, 0.9), 2);
  });

  it('weights the channels like the board', () => {
    const red = new Uint8ClampedArray([255, 0, 0, 255]);
    const green = new Uint8ClampedArray([0, 255, 0, 255]);

    expect(ditherToEpaper(red, 1, 1)[0]).toBe(0);
    expect(ditherToEpaper(green, 1, 1)[0]).toBe(255);
  });

});

describe('applyToneCurve', () => {
  const curve = (values: number[]) => {
    const gray = Float32Array.from(values);
    applyToneCurve(gray);
    return Array.from(gray);
  };

  it('stretches the darkest and brightest tones to black and white', () => {
    const [darkest, , , brightest] = curve([50, 100, 150, 200]);

    expect(darkest).toBe(0);
    expect(brightest).toBe(255);
  });

  it('lifts the midtones like the board', () => {
    // (100 - 50) / 150 = 1/3, raised to 0.9
    expect(curve([50, 100, 150, 200])[1]).toBeCloseTo(Math.pow(1 / 3, 0.9) * 255, 3);
  });

  it('leaves a near-flat image unstretched', () => {
    const [low, , high] = curve([100, 110, 120]);

    expect(low).toBeCloseTo(Math.pow(100 / 255, 0.9) * 255, 3);
    expect(high).toBeCloseTo(Math.pow(120 / 255, 0.9) * 255, 3);
  });
});

describe('ditherSerpentine', () => {
  const dither = (values: number[], width: number, height: number) => {
    const gray = Float32Array.from(values);
    ditherSerpentine(gray, width, height);
    return Array.from(gray);
  };

  it('diffuses the error like the board, dropping what falls off the edges', () => {
    // Traced by hand through 7/3/5/1 sixteenths: e.g. (0,0)=100 → black, pushing 43.75 right
    // and 31.25 down, which lifts (0,1) from 120 over the threshold.
    expect(dither([
      100, 200, 50, 150,
      120, 60, 220, 130,
    ], 4, 2)).toEqual([
      0, 255, 0, 255,
      255, 0, 255, 0,
    ]);
  });

  // Left to right, the second row would come out 0, 255, 0.
  it('runs odd rows right to left with the kernel mirrored', () => {
    expect(dither([
      80, 140, 140,
      80, 100, 140,
    ], 3, 2)).toEqual([
      0, 255, 0,
      0, 0, 255,
    ]);
  });
});
