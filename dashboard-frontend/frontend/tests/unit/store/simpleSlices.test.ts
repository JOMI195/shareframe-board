import { afterEach, describe, expect, it, vi } from 'vitest';
import navigation, {
  closeSidebar,
  getAdvancedMode,
  getCurrentPath,
  getSidebar,
  openSidedbar,
  setCurrentPath,
  toggleAdvancedMode,
} from '@/store/navigation/navigation.Slice';
import loadingWall, { hideLoadingWall, showLoadingWall } from '@/store/loadingWall/loadingWall.Slice';
import frameLogs, { clearLogs, fetchFrameLogs } from '@/store/frameLogs/frameLogs.Slice';
import slideshowStatus, { checkSlideshowStatusThunk, resetStatus } from '@/store/slideshowStatus/slideshowStatus.Slice';
import { ServiceType } from '@/types';
import { rootState } from '@tests/helpers/preloadedState';

const init = { type: '@@tests/INIT' };

describe('navigation slice', () => {
  it('opens and closes the sidebar', () => {
    const opened = navigation(undefined, openSidedbar());
    expect(opened.sidebar.open).toBe(true);
    expect(navigation(opened, closeSidebar()).sidebar.open).toBe(false);
  });

  it('tracks the current path and toggles advanced mode', () => {
    let state = navigation(undefined, setCurrentPath('/network/'));
    state = navigation(state, toggleAdvancedMode());

    const root = { ...rootState(), navigation: state };
    expect(getCurrentPath(root)).toBe('/network/');
    expect(getAdvancedMode(root)).toBe(true);
    expect(getSidebar(root)).toEqual({ open: false });

    expect(navigation(state, toggleAdvancedMode()).advancedMode).toBe(false);
  });
});

describe('loadingWall slice', () => {
  afterEach(() => vi.useRealTimers());

  it('shows the wall with a three minute deadline', () => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2026-10-01T12:00:00Z'));

    const state = loadingWall(undefined, showLoadingWall('Bitte warten'));

    expect(state).toEqual({ isLoadingWallVisible: true, message: 'Bitte warten', hideAfter: Date.now() + 180_000 });
  });

  it('hides the wall and forgets the deadline', () => {
    const shown = loadingWall(undefined, showLoadingWall('Bitte warten'));

    expect(loadingWall(shown, hideLoadingWall())).toEqual({ isLoadingWallVisible: false, message: '', hideAfter: undefined });
  });
});

describe('frameLogs slice', () => {
  const params = { service_name: ServiceType.DISPLAY, lines: 100 };
  const logsData = { service: 'display', period: '', timestamp: '', log_count: 1, logs: ['x'] };

  it('remembers the params of a successful fetch for the cache', () => {
    const state = frameLogs(undefined, fetchFrameLogs.fulfilled(logsData, 'req', params));

    expect(state).toMatchObject({ loading: false, logsData, cachedParams: JSON.stringify(params), error: null });
    expect(state.lastFetchTimestamp).toEqual(expect.any(Number));
  });

  it('stores the rejection reason', () => {
    const state = frameLogs(undefined, fetchFrameLogs.rejected(null, 'req', params, 'kaputt'));

    expect(state).toMatchObject({ loading: false, error: 'kaputt' });
  });

  it('clears logs and cache', () => {
    const filled = frameLogs(undefined, fetchFrameLogs.fulfilled(logsData, 'req', params));

    expect(frameLogs(filled, clearLogs())).toEqual(frameLogs(undefined, init));
  });
});

describe('slideshowStatus slice', () => {
  const payload = { active: true, loopStarted: true, imageCount: 4, secondsUntilNext: 30 };

  it('applies a status check', () => {
    const state = slideshowStatus(undefined, checkSlideshowStatusThunk.fulfilled(payload, 'req'));

    expect(state).toMatchObject({ isActive: true, loopStarted: true, imageCount: 4, secondsUntilNext: 30, isLoading: false });
    expect(state.lastCheckedAt).toEqual(expect.any(Number));
  });

  it('treats a failed check as stopped', () => {
    const active = slideshowStatus(undefined, checkSlideshowStatusThunk.fulfilled(payload, 'req'));
    const state = slideshowStatus(active, checkSlideshowStatusThunk.rejected(null, 'req', undefined, 'offline'));

    expect(state).toMatchObject({ isActive: false, loopStarted: false, imageCount: null, secondsUntilNext: null, error: 'offline' });
  });

  it('resets', () => {
    const active = slideshowStatus(undefined, checkSlideshowStatusThunk.fulfilled(payload, 'req'));

    expect(slideshowStatus(active, resetStatus())).toEqual(slideshowStatus(undefined, init));
  });
});
