import { describe, expect, it } from 'vitest';
import { getBadgeNumber } from '@/common/utils/badge';

describe('getBadgeNumber', () => {
  it('counts the true conditions', () => {
    expect(getBadgeNumber(true, false, true)).toBe(2);
  });

  it('is zero without conditions', () => {
    expect(getBadgeNumber()).toBe(0);
    expect(getBadgeNumber(false, false)).toBe(0);
  });
});
