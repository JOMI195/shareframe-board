import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { isStaleChunkError, reloadForStaleChunk } from '@/common/utils/staleChunkReload';

describe('stale chunk reload', () => {
  const reload = vi.fn();

  beforeEach(() => {
    sessionStorage.clear();
    vi.useFakeTimers();
    vi.stubGlobal('location', { ...window.location, reload });
  });

  afterEach(() => {
    vi.useRealTimers();
    vi.unstubAllGlobals();
    reload.mockReset();
  });

  it('tells a chunk lost to an update from other errors', () => {
    expect(isStaleChunkError(new TypeError('Failed to fetch dynamically imported module: /assets/x.js'))).toBe(true);
    expect(isStaleChunkError(new Error('Importing a module script failed.'))).toBe(true);
    expect(isStaleChunkError(new Error('Network error'))).toBe(false);
    expect(isStaleChunkError('dynamically imported module')).toBe(false);
  });

  it('reloads once, and again only after 30 seconds, so a missing chunk cannot loop the page', () => {
    expect(reloadForStaleChunk()).toBe(true);
    expect(reloadForStaleChunk()).toBe(false);
    vi.advanceTimersByTime(30_000);
    expect(reloadForStaleChunk()).toBe(true);
    expect(reload).toHaveBeenCalledTimes(2);
  });
});
