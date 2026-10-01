import { act } from '@testing-library/react';
import { vi } from 'vitest';

// The layout locks the slideshow actions for 20s after mount, and every action
// locks them again for 3 minutes to protect the e-paper panel.
export const INITIAL_LOAD_LOCK_MS = 20_000;

export const useBoardClock = () => vi.useFakeTimers({ shouldAdvanceTime: true });

export const advance = (ms: number) => act(() => vi.advanceTimersByTimeAsync(ms));
