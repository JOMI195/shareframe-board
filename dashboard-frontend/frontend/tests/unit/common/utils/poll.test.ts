import { afterEach, describe, expect, it, vi } from 'vitest';
import { pollWhileVisible } from '@/common/utils/poll';

const setVisibility = (state: DocumentVisibilityState) => {
  Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => state });
  document.dispatchEvent(new Event('visibilitychange'));
};

describe('pollWhileVisible', () => {
  afterEach(() => {
    setVisibility('visible');
    vi.useRealTimers();
  });

  it('asks at once and then on every interval', () => {
    vi.useFakeTimers();
    const ask = vi.fn();
    const stop = pollWhileVisible(ask, 1000);

    expect(ask).toHaveBeenCalledTimes(1);
    vi.advanceTimersByTime(3000);
    expect(ask).toHaveBeenCalledTimes(4);
    stop();
  });

  // A phone keeps a hidden tab around for hours; the board should not answer it all that time.
  it('pauses in a hidden tab and catches up when it is shown again', () => {
    vi.useFakeTimers();
    const ask = vi.fn();
    const stop = pollWhileVisible(ask, 1000);

    setVisibility('hidden');
    vi.advanceTimersByTime(60_000);
    expect(ask).toHaveBeenCalledTimes(1);

    setVisibility('visible');
    expect(ask).toHaveBeenCalledTimes(2);
    vi.advanceTimersByTime(1000);
    expect(ask).toHaveBeenCalledTimes(3);
    stop();
  });

  it('stops for good when told', () => {
    vi.useFakeTimers();
    const ask = vi.fn();
    pollWhileVisible(ask, 1000)();

    setVisibility('hidden');
    setVisibility('visible');
    vi.advanceTimersByTime(5000);
    expect(ask).toHaveBeenCalledTimes(1);
  });
});
