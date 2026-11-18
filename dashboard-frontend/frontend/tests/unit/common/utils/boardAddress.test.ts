import { describe, expect, it } from 'vitest';
import { boardAddress } from '@/common/utils/boardAddress';

describe('boardAddress', () => {
  it('is the hotspot name as mDNS host, the same name the board takes', () => {
    expect(boardAddress('shareframe-board-C3D4')).toBe('http://shareframe-board-C3D4.local');
  });

  it('falls back to the default name when the hotspot name is unknown', () => {
    expect(boardAddress('')).toBe('http://shareframe-board.local');
  });
});
