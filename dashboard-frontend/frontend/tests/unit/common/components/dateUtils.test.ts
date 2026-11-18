import { describe, expect, it } from 'vitest';
import { formatGermanDateTime } from '@/common/components/dateUtils';

describe('formatGermanDateTime', () => {
  it('writes day, month, year and the time the German way', () => {
    expect(formatGermanDateTime(new Date('2026-03-07T09:05:03Z'))).toBe('07.03.2026, 09:05:03');
  });

  it('takes an ISO string from the board as well', () => {
    expect(formatGermanDateTime('2026-12-31T23:59:59Z' as unknown as Date)).toBe('31.12.2026, 23:59:59');
  });
});
