import { describe, expect, it } from 'vitest';
import { formatUptime } from '@/main/services/formatUptime';

describe('formatUptime', () => {
  it.each([
    [null, '–'],
    [0, '0s'],
    [59, '59s'],
    [60, '1m'],
    [3599, '59m'],
    [3600, '1h 0m'],
    [3 * 3600 + 25 * 60, '3h 25m'],
    [86400, '1d 0h'],
    [2 * 86400 + 5 * 3600 + 59 * 60, '2d 5h'],
  ])('formats %s as %s', (seconds, expected) => {
    expect(formatUptime(seconds)).toBe(expected);
  });
});
