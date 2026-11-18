import { afterEach, describe, expect, it, vi } from 'vitest';
import { clearLocalDataAndReload } from '@/common/utils/localData';

describe('clearLocalDataAndReload', () => {
  afterEach(() => vi.unstubAllGlobals());

  it('drops what the browser keeps for the dashboard, then reloads', async () => {
    const reload = vi.fn();
    vi.stubGlobal('location', { ...window.location, reload });
    localStorage.setItem('persist:root', '{}');
    sessionStorage.setItem('stale-chunk-reload', '1');

    await clearLocalDataAndReload();

    expect(localStorage.length).toBe(0);
    expect(sessionStorage.length).toBe(0);
    expect(reload).toHaveBeenCalledOnce();
  });

  it('reloads even when clearing fails', async () => {
    const reload = vi.fn();
    vi.stubGlobal('location', { ...window.location, reload });
    vi.spyOn(Storage.prototype, 'clear').mockImplementationOnce(() => { throw new Error('blocked'); });

    await expect(clearLocalDataAndReload()).rejects.toThrow('blocked');
    expect(reload).toHaveBeenCalledOnce();
    vi.restoreAllMocks();
  });
});
