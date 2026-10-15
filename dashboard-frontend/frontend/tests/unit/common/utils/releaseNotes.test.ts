import { describe, expect, it } from 'vitest';
import { parseReleaseNotes } from '@/common/utils/releaseNotes';

describe('parseReleaseNotes', () => {
  it('groups bullet lines into one list without markers and blank lines', () => {
    expect(parseReleaseNotes('- QR-Code im AP-Modus\n\n* Stabilitätsupdates\r\n')).toEqual([
      { type: 'list', items: ['QR-Code im AP-Modus', 'Stabilitätsupdates'] },
    ]);
  });

  it('keeps plain lines as paragraphs between lists', () => {
    expect(parseReleaseNotes('Neu:\n- A\nBehoben:\n- B')).toEqual([
      { type: 'paragraph', text: 'Neu:' },
      { type: 'list', items: ['A'] },
      { type: 'paragraph', text: 'Behoben:' },
      { type: 'list', items: ['B'] },
    ]);
  });

  it('does not treat a hyphenated word as a bullet', () => {
    expect(parseReleaseNotes('-Fix')).toEqual([{ type: 'paragraph', text: '-Fix' }]);
  });

  it('is empty for whitespace-only notes', () => {
    expect(parseReleaseNotes('  \n\n ')).toEqual([]);
  });
});
