import { describe, expect, it } from 'vitest';
import { formatTimeLeft, formatUnix } from '@/main/slideshowImages/slideshowImagesUtils';

const NOW = Date.UTC(2025, 9, 10, 12, 0, 0);
const inSecs = (secs: number) => NOW / 1000 + secs;

describe('slideshow image time formatting', () => {
  it('names the time a picture has left in days, hours and minutes', () => {
    expect(formatTimeLeft(inSecs(86_400 + 3_600 + 60), NOW)).toBe('noch 1 Tag 1 Stunde 1 Minute');
    expect(formatTimeLeft(inSecs(2 * 86_400 + 5 * 60), NOW)).toBe('noch 2 Tage 5 Minuten');
    expect(formatTimeLeft(inSecs(20), NOW)).toBe('noch weniger als 1 Minute');
  });

  it('calls a picture past its expiry expired', () => {
    expect(formatTimeLeft(inSecs(0), NOW)).toBe('Abgelaufen');
    expect(formatTimeLeft(inSecs(-3_600), NOW)).toBe('Abgelaufen');
  });

  it('shows no date for a picture stored before arrival times were recorded', () => {
    expect(formatUnix(null)).toBe('–');
  });
});
