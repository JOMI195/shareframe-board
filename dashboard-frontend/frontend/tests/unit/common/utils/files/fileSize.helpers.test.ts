import { describe, expect, it } from 'vitest';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';

describe('getReadablyFileSize', () => {
  it.each([
    [0, '0 Bytes'],
    [900, '900 Bytes'],
    [901, '0.9 KB'],
    [1_500_000, '1.5 MB'],
    [2_345_678_901, '2.35 GB'],
  ])('%d bytes reads as %s', (bytes, text) => {
    expect(getReadablyFileSize(bytes)).toBe(text);
  });
});
