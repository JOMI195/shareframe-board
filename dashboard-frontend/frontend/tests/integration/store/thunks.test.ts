import { afterEach, describe, expect, it, vi } from 'vitest';
import { http, HttpResponse } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { checkAuthStatusThunk } from '@/store/auth/auth.Slice';
import { fetchConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { fetchFrameLogs } from '@/store/frameLogs/frameLogs.Slice';
import { fetchNetworkData } from '@/store/network/network.Slice';
import { restartPi } from '@/store/piPower/piPower.Slice';
import { fetchServices } from '@/store/services/services.Slice';
import {
  fetchDisplayImagesLoopInterval,
  fetchNightMode,
  setToggleStatus,
  toggleSlideshowThunk,
  updateDisplayImagesLoopInterval,
} from '@/store/slideshowOperation/slideshowOperation.Slice';
import { checkSlideshowStatusThunk, startContinuousStatusCheck } from '@/store/slideshowStatus/slideshowStatus.Slice';
import type { AppDispatch } from '@/store';
import { setupStore } from '@/store/setupStore';
import { ServiceType } from '@/types';
import { advance, useBoardClock } from '@tests/helpers/time';
import {
  apiUrl,
  failsWith,
  ok,
  server,
  spyOnRequests,
  unreachable,
  withSlideshowStatus,
} from '@tests/mocks';

// The app's plain thunks are typed against the persisted store's dispatch.
const boardStore = (...args: Parameters<typeof setupStore>) => {
  const store = setupStore(...args);
  return { ...store, dispatch: store.dispatch as AppDispatch };
};

const alerts = (store: ReturnType<typeof boardStore>) =>
  store.getState().snackbars.snackbars.alerts.map((a) => a.message);

afterEach(() => vi.useRealTimers());

describe('session check', () => {
  it('treats an answer without the flag as signed out', async () => {
    server.use(http.get(apiUrl(api.getAuthStatusUrl()), () => ok({})));
    const store = boardStore();

    await store.dispatch(checkAuthStatusThunk());

    expect(store.getState().auth.isAuthenticated).toBe(false);
  });

  it('signs out when the board does not answer', async () => {
    server.use(...unreachable('get', api.getAuthStatusUrl()));
    const store = boardStore({ auth: { isAuthenticated: true, isLoading: false, error: null } });

    await store.dispatch(checkAuthStatusThunk());

    expect(store.getState().auth.isAuthenticated).toBe(false);
  });
});

describe('connection mode', () => {
  it('stays unloaded on an unexpected answer', async () => {
    server.use(...failsWith('get', api.getConnectionModeUrl()));
    const store = boardStore();

    await store.dispatch(fetchConnectionMode());

    expect(store.getState().connectionMode.loaded).toBe(false);
  });
});

describe('network data', () => {
  it('reports an unreachable board', async () => {
    server.use(...unreachable('get', api.getConnectionStatusUrl()));
    const store = boardStore();

    await store.dispatch(fetchNetworkData());

    expect(alerts(store)).toEqual(['Aktualisieren fehlgeschlagen']);
    expect(store.getState().network.error).toBeTruthy();
  });

  // Only a thrown request alerts; an explicit failure is silent.
  it('rejects quietly when one of both lists fails', async () => {
    server.use(...failsWith('get', api.getConnectionSavedNetworksUrl()));
    const store = boardStore();

    await store.dispatch(fetchNetworkData());

    expect(store.getState().network.error).toBe('Failed to fetch network data');
    expect(alerts(store)).toEqual([]);
  });
});

describe('services', () => {
  it('keeps the reason of a failed fetch', async () => {
    server.use(...failsWith('get', api.getServicesUrl(), 500, 'IPC down'));
    const store = boardStore();

    await store.dispatch(fetchServices());

    expect(store.getState().services.error).toBe('IPC down');
  });
});

describe('logs', () => {
  const params = { service_name: ServiceType.DISPLAY, lines: 10 };

  it('serves the same query from cache for 30 seconds', async () => {
    useBoardClock();
    const logs = spyOnRequests('get', api.getFrameLogsUrl(), () => ok({ service_name: 'display', lines: 1, logs: 'a\n' }));
    server.use(logs.handler);
    const store = boardStore();

    await store.dispatch(fetchFrameLogs(params));
    await store.dispatch(fetchFrameLogs(params));
    expect(logs.calls).toHaveLength(1);

    await store.dispatch(fetchFrameLogs({ ...params, lines: 20 }));
    expect(logs.calls).toHaveLength(2);

    vi.advanceTimersByTime(30_000);
    await store.dispatch(fetchFrameLogs({ ...params, lines: 20 }));
    expect(logs.calls).toHaveLength(3);
  });

  it('splits the log text into lines', async () => {
    server.use(http.get(apiUrl(api.getFrameLogsUrl()), () => ok({ service_name: 'display', lines: 2, logs: 'one\n\ntwo\n' })));
    const store = boardStore();

    await store.dispatch(fetchFrameLogs(params));

    expect(store.getState().frameLogs.logsData).toMatchObject({ service: 'display', logs: ['one', 'two'], log_count: 2 });
  });
});

describe('slideshow status', () => {
  it('maps negative counters to unknown', async () => {
    server.use(...withSlideshowStatus({ seconds_until_next: -1, image_count: -1 }));
    const store = boardStore();

    await store.dispatch(checkSlideshowStatusThunk());

    expect(store.getState().slideshowStatus).toMatchObject({ secondsUntilNext: null, imageCount: null, isActive: true });
  });

  it('polls every five seconds until stopped', async () => {
    useBoardClock();
    const status = spyOnRequests('get', api.getSlideshowStatusUrl(), () => ok({ active: true }));
    server.use(status.handler);
    const store = boardStore();

    const stop = store.dispatch(startContinuousStatusCheck());
    await advance(10_000);
    stop();
    await advance(10_000);

    expect(status.calls).toHaveLength(3);
  });
});

describe('slideshow settings', () => {
  it('reads interval and night mode from the status', async () => {
    server.use(
      ...withSlideshowStatus({
        interval_seconds: 1800,
        night_mode: { enabled: true, start_hour: 23, end_hour: 6, interval_seconds: 7200, active_now: false },
      }),
    );
    const store = boardStore();

    await store.dispatch(fetchDisplayImagesLoopInterval());
    await store.dispatch(fetchNightMode());

    expect(store.getState().slideshowOperation).toMatchObject({
      displayImagesIntervalMins: 30,
      nightModeEnabled: true,
      nightStartHour: 23,
      nightEndHour: 6,
      nightIntervalMins: 120,
      isFetchingInterval: false,
      isFetchingNightMode: false,
    });
  });

  it('reports when the settings cannot be read', async () => {
    server.use(...failsWith('get', api.getSlideshowStatusUrl()));
    const store = boardStore();

    await store.dispatch(fetchDisplayImagesLoopInterval());
    await store.dispatch(fetchNightMode());

    expect(alerts(store)).toEqual([
      'Abrufen des Intervalls der Bilderwiedergabe fehlgeschlagen',
      'Abrufen des Nachtmodus fehlgeschlagen',
    ]);
  });

  it('does not change the interval while another action runs', async () => {
    const interval = spyOnRequests('post', api.getSlideshowIntervalUrl());
    server.use(interval.handler);
    const store = boardStore();
    store.dispatch(setToggleStatus({ isToggling: true }));

    await store.dispatch(updateDisplayImagesLoopInterval(30));

    expect(interval.calls).toHaveLength(0);
  });
});

describe('slideshow toggle', () => {
  it('gives up after ten minutes when the board never switches', async () => {
    useBoardClock();
    server.use(
      ...withSlideshowStatus({ active: true }),
      http.post(apiUrl(api.getSlideshowUrl()), () => ok({ action: 'stop' })),
    );
    const store = boardStore();

    const done = store.dispatch(toggleSlideshowThunk());
    await advance(10 * 60_000 + 2_000);
    await done;

    expect(alerts(store)).toContain('Zeitüberschreitung bei Statusänderung');
    expect(store.getState().slideshowOperation.isToggling).toBe(false);
  });

  it('reports a refused toggle', async () => {
    server.use(...failsWith('post', api.getSlideshowUrl()));
    const store = boardStore();

    await store.dispatch(toggleSlideshowThunk());

    expect(alerts(store)).toEqual(['Fehler bei der Statusänderung der Bildwiedergabe']);
    expect(store.getState().slideshowOperation.error).toBe('Statusänderung fehlgeschlagen');
  });
});

describe('restart', () => {
  // No HTTP answer is the expected outcome of a power command, not a failure.
  it('counts a dropped connection as accepted', async () => {
    useBoardClock();
    server.use(
      http.post(apiUrl(api.getRestartPiUrl()), () => HttpResponse.error()),
      ...unreachable('get', api.getSystemHealthUrl()),
    );
    const store = boardStore();
    const navigate = vi.fn();

    const done = store.dispatch(restartPi(navigate));
    await advance(4 * 60_000);
    await done;

    expect(navigate).toHaveBeenCalledWith('/');
    expect(alerts(store)).toEqual(['Neustartprozess gestartet']);
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(true);
    expect(window.location.reload).not.toHaveBeenCalled();
  });
});
