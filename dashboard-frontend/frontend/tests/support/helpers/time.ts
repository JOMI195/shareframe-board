import { act } from '@testing-library/react';
import { vi } from 'vitest';

export const useBoardClock = () => vi.useFakeTimers({ shouldAdvanceTime: true });

export const advance = (ms: number) => act(() => vi.advanceTimersByTimeAsync(ms));
