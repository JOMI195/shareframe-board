import { describe, expect, it } from 'vitest';
import { isVersionNewer } from '@/common/utils/version';

describe('isVersionNewer', () => {
  it.each([
    ['2.0.0', '1.9.9'],
    ['1.3.0', '1.2.9'],
    ['1.2.4', '1.2.3'],
  ])('%s is newer than %s', (latest, current) => {
    expect(isVersionNewer(latest, current)).toBe(true);
  });

  it.each([
    ['1.2.3', '1.2.3'],
    ['1.2.2', '1.2.3'],
    ['1.1.9', '1.2.0'],
    ['0.9.9', '1.0.0'],
  ])('%s is not newer than %s', (latest, current) => {
    expect(isVersionNewer(latest, current)).toBe(false);
  });

  it('is false when either side is missing', () => {
    expect(isVersionNewer('', '1.0.0')).toBe(false);
    expect(isVersionNewer('1.0.0', '')).toBe(false);
  });
});
