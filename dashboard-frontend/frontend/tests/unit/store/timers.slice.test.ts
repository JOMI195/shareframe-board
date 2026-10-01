import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import reducer, {
  addTimer,
  formatTime,
  removeTimer,
  resetAllTimers,
  resetTimer,
  selectActiveTimers,
  selectTimer,
  startTimer,
  stopTimer,
  syncSpecificTimer,
  syncTimers,
  updateTimer,
} from '@/store/timers/timers.Slice';
import { setupStore } from '@/store/setupStore';
import { rootState } from '@tests/helpers/preloadedState';

const initial = () => reducer(undefined, { type: '@@tests/INIT' });

describe('timers slice', () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2026-10-01T12:00:00Z'));
  });

  afterEach(() => vi.useRealTimers());

  it('creates a timer with the default three minutes', () => {
    const state = reducer(initial(), addTimer({ id: 't' }));

    expect(state.timers.t).toEqual({ id: 't', duration: 180, remaining: 180, isActive: false });
  });

  it('does not overwrite an existing timer', () => {
    let state = reducer(initial(), addTimer({ id: 't', duration: 10 }));
    state = reducer(state, updateTimer({ id: 't', remaining: 4 }));
    state = reducer(state, addTimer({ id: 't', duration: 99 }));

    expect(state.timers.t).toMatchObject({ duration: 10, remaining: 4 });
  });

  it('starts, stops and resets', () => {
    let state = reducer(initial(), addTimer({ id: 't', duration: 10 }));
    state = reducer(state, startTimer('t'));
    expect(state.timers.t).toMatchObject({ isActive: true, startTimestamp: Date.now() });

    state = reducer(state, updateTimer({ id: 't', remaining: 3 }));
    state = reducer(state, stopTimer('t'));
    expect(state.timers.t).toMatchObject({ isActive: false, startTimestamp: undefined, remaining: 3 });

    state = reducer(state, resetTimer('t'));
    expect(state.timers.t).toMatchObject({ remaining: 10, isActive: false });
  });

  it('does not restart a running timer', () => {
    let state = reducer(initial(), addTimer({ id: 't', duration: 10 }));
    state = reducer(state, startTimer('t'));
    const startedAt = state.timers.t.startTimestamp;

    vi.advanceTimersByTime(5_000);
    state = reducer(state, startTimer('t'));

    expect(state.timers.t.startTimestamp).toBe(startedAt);
  });

  it('stops a timer that reaches zero', () => {
    let state = reducer(initial(), addTimer({ id: 't', duration: 10 }));
    state = reducer(state, startTimer('t'));
    state = reducer(state, updateTimer({ id: 't', remaining: -2 }));

    expect(state.timers.t).toMatchObject({ remaining: 0, isActive: false });
  });

  it('ignores actions for unknown timers', () => {
    const state = initial();

    expect(reducer(state, startTimer('nope'))).toEqual(state);
    expect(reducer(state, updateTimer({ id: 'nope', remaining: 1 }))).toEqual(state);
  });

  it('removes one timer and resets all', () => {
    let state = reducer(initial(), addTimer({ id: 'a', duration: 5 }));
    state = reducer(state, addTimer({ id: 'b', duration: 7 }));
    state = reducer(state, updateTimer({ id: 'b', remaining: 1 }));
    state = reducer(state, removeTimer('a'));
    state = reducer(state, resetAllTimers());

    expect(Object.keys(state.timers)).toEqual(['b']);
    expect(state.timers.b.remaining).toBe(7);
  });

  it('selects timers', () => {
    let timers = reducer(initial(), addTimer({ id: 'a' }));
    timers = reducer(timers, addTimer({ id: 'b' }));
    timers = reducer(timers, startTimer('b'));
    const state = { ...rootState(), timers };

    expect(selectTimer(state, 'a')?.id).toBe('a');
    expect(selectActiveTimers(state).map((t) => t.id)).toEqual(['b']);
  });

  it('syncs a running timer against the wall clock', async () => {
    const store = setupStore();
    store.dispatch(addTimer({ id: 't', duration: 10 }));
    store.dispatch(startTimer('t'));

    vi.advanceTimersByTime(4_000);
    await store.dispatch(syncSpecificTimer('t'));
    expect(store.getState().timers.timers.t.remaining).toBe(6);

    vi.advanceTimersByTime(20_000);
    await store.dispatch(syncSpecificTimer('t'));
    expect(store.getState().timers.timers.t).toMatchObject({ remaining: 0, isActive: false });
  });

  it('syncs all running timers by the time since the last sync', async () => {
    const store = setupStore();
    store.dispatch(addTimer({ id: 'run', duration: 10 }));
    store.dispatch(addTimer({ id: 'idle', duration: 10 }));
    store.dispatch(startTimer('run'));

    vi.advanceTimersByTime(3_000);
    await store.dispatch(syncTimers());

    expect(store.getState().timers.timers.run.remaining).toBe(7);
    expect(store.getState().timers.timers.idle.remaining).toBe(10);
    expect(store.getState().timers.lastSyncTime).toBe(Date.now());
  });

  it.each([
    [0, '00:00'],
    [5, '00:05'],
    [65, '01:05'],
    [600, '10:00'],
  ])('formats %i seconds as %s', (seconds, expected) => {
    expect(formatTime(seconds)).toBe(expected);
  });
});
