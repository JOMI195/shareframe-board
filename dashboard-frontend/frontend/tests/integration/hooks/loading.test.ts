import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { act, renderHook } from '@testing-library/react';
import { useDelayedLoading } from '@/hooks/loading/useDelayedLoading';
import { useMinimumLoading } from '@/hooks/loading/useMinimumLoading';

describe('loading indicators', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('a fast request never shows the delayed spinner, a slow one does', () => {
    const { result, rerender } = renderHook(({ loading }) => useDelayedLoading(loading, 600), { initialProps: { loading: true } });
    act(() => vi.advanceTimersByTime(599));
    expect(result.current).toBe(false);
    act(() => vi.advanceTimersByTime(1));
    expect(result.current).toBe(true);
    rerender({ loading: false });
    expect(result.current).toBe(false);
  });

  it('a fast response keeps the skeleton up for the minimum, a slow one hides it at once', () => {
    const { result, rerender } = renderHook(({ loading }) => useMinimumLoading(loading, 400), { initialProps: { loading: true } });
    act(() => vi.advanceTimersByTime(100));
    rerender({ loading: false });
    expect(result.current).toBe(true);
    act(() => vi.advanceTimersByTime(300));
    expect(result.current).toBe(false);

    rerender({ loading: true });
    act(() => vi.advanceTimersByTime(500));
    rerender({ loading: false });
    expect(result.current).toBe(false);
  });
});
