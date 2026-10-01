import { afterEach, describe, expect, it, vi } from 'vitest';
import reducer, {
  resetOperation,
  selectIsAnyOperationActive,
  setClearDisplayStatus,
  setDisplayRefreshInterval,
  setError,
  setFetchIntervalStatus,
  setNightMode,
  setSkipImageStatus,
  setToggleStatus,
  setUpdateIntervalStatus,
  setUpdateNightModeStatus,
} from '@/store/slideshowOperation/slideshowOperation.Slice';
import { rootState } from '@tests/helpers/preloadedState';

const initial = () => reducer(undefined, { type: '@@tests/INIT' });

describe('slideshowOperation slice', () => {
  afterEach(() => vi.useRealTimers());

  it.each([
    ['toggle', setToggleStatus({ isToggling: true })],
    ['clear', setClearDisplayStatus({ isClearingDisplay: true })],
    ['skip', setSkipImageStatus({ isSkippingImage: true })],
    ['interval', setUpdateIntervalStatus({ isUpdatingInterval: true })],
    ['night mode', setUpdateNightModeStatus({ isUpdatingNightMode: true })],
  ])('a running %s operation blocks the others and stamps its start', (_name, action) => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2026-10-01T12:00:00Z'));

    const state = reducer(initial(), action);

    expect(state.startTime).toBe(Date.now());
    expect(selectIsAnyOperationActive({ ...rootState(), slideshowOperation: state })).toBe(true);
  });

  it('fetching the interval is not an operation', () => {
    const state = reducer(initial(), setFetchIntervalStatus({ isFetchingInterval: true }));

    expect(selectIsAnyOperationActive({ ...rootState(), slideshowOperation: state })).toBe(false);
  });

  it('clears the start time when an operation ends', () => {
    let state = reducer(initial(), setToggleStatus({ isToggling: true }));
    state = reducer(state, setToggleStatus({ isToggling: false }));

    expect(state.startTime).toBeNull();
  });

  it('stores interval and night mode', () => {
    let state = reducer(initial(), setDisplayRefreshInterval(30));
    state = reducer(state, setNightMode({ enabled: true, startHour: 22, endHour: 6, intervalMins: 120, activeNow: true }));

    expect(state).toMatchObject({
      displayImagesIntervalMins: 30,
      nightModeEnabled: true,
      nightStartHour: 22,
      nightEndHour: 6,
      nightIntervalMins: 120,
      nightModeActiveNow: true,
    });
  });

  it('keeps the active-now flag when the update does not report it', () => {
    let state = reducer(initial(), setNightMode({ enabled: true, startHour: 1, endHour: 2, intervalMins: 60, activeNow: true }));
    state = reducer(state, setNightMode({ enabled: false, startHour: 1, endHour: 2, intervalMins: 60 }));

    expect(state.nightModeActiveNow).toBe(true);
  });

  it('resets every operation flag but keeps the settings', () => {
    let state = reducer(initial(), setDisplayRefreshInterval(30));
    state = reducer(state, setSkipImageStatus({ isSkippingImage: true }));
    state = reducer(state, setError('kaputt'));
    state = reducer(state, resetOperation());

    expect(state).toEqual({ ...initial(), displayImagesIntervalMins: 30 });
  });
});
