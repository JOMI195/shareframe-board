import { describe, expect, it } from 'vitest';
import { PASSEPARTOUT, safeArea } from '@/main/pictureUpload/passepartout';

describe('safeArea', () => {
  it('leaves the visible window of the frame', () => {
    const safe = safeArea(800, 480);

    expect(safe.x).toBeCloseTo(800 * PASSEPARTOUT.x);
    expect(safe.y).toBeCloseTo(480 * PASSEPARTOUT.y);
    expect(safe.x + safe.width / 2).toBeCloseTo(400);
    expect(safe.y + safe.height / 2).toBeCloseTo(240);
  });

  it('keeps the same share of a photo of another resolution', () => {
    const small = safeArea(800, 480);
    const large = safeArea(1600, 960);

    expect(large.width).toBeCloseTo(small.width * 2);
    expect(large.height).toBeCloseTo(small.height * 2);
  });

  it('covers more of the width than of the height', () => {
    expect(PASSEPARTOUT.x).toBeGreaterThan(PASSEPARTOUT.y);
  });
});
