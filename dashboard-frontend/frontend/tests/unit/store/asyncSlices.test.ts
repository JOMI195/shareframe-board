import { describe, expect, it } from 'vitest';
import auth, { checkAuthStatusThunk, loginThunk, logoutThunk, resetAuthState } from '@/store/auth/auth.Slice';
import network, { addNetwork, fetchNetworkData, forgetNetwork } from '@/store/network/network.Slice';
import connectionMode, { fetchConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import updates, { fetchLatestRelease, fetchUpdateHistory, fetchUpdateStatus } from '@/store/updates/updates.Slice';
import services, { fetchServices, restartService, selectService } from '@/store/services/services.Slice';
import frameInfo, { fetchFrameInfos } from '@/store/frameInfo/frameInfo.Slice';
import displayStats, { fetchDisplayStats } from '@/store/displayStats/displayStats.Slice';
import { rootState } from '@tests/helpers/preloadedState';
import {
  healthyServices,
  makeDisplayStats,
  makeFrameInfo,
  makeRelease,
  makeService,
  makeUpdateHistoryEntry,
  makeUpdateStatus,
} from '@tests/fixtures';

describe('auth slice', () => {
  it('follows the status check', () => {
    const pending = auth(undefined, checkAuthStatusThunk.pending('req'));
    expect(pending.isLoading).toBe(true);

    expect(auth(pending, checkAuthStatusThunk.fulfilled(true, 'req'))).toEqual({ isAuthenticated: true, isLoading: false, error: null });
  });

  it('signs out when the status check fails', () => {
    const signedIn = auth(undefined, checkAuthStatusThunk.fulfilled(true, 'req'));

    const state = auth(signedIn, checkAuthStatusThunk.rejected(new Error('offline'), 'req'));

    expect(state).toEqual({ isAuthenticated: false, isLoading: false, error: 'offline' });
  });

  it('signs in on a successful login and keeps the reason of a failed one', () => {
    expect(auth(undefined, loginThunk.fulfilled(true, 'req', { otp: '123' })).isAuthenticated).toBe(true);

    const failed = auth(undefined, loginThunk.rejected(null, 'req', { password: 'x' }, 'Passwort ungültig'));
    expect(failed).toEqual({ isAuthenticated: false, isLoading: false, error: 'Passwort ungültig' });
  });

  it('signs out on logout, but stays signed in when logout fails', () => {
    const signedIn = auth(undefined, loginThunk.fulfilled(true, 'req', { otp: '1' }));

    expect(auth(signedIn, logoutThunk.fulfilled(true, 'req')).isAuthenticated).toBe(false);
    expect(auth(signedIn, logoutThunk.rejected(new Error('x'), 'req')).isAuthenticated).toBe(true);
  });

  it('resets', () => {
    const signedIn = auth(undefined, loginThunk.fulfilled(true, 'req', { otp: '1' }));

    expect(auth(signedIn, resetAuthState())).toEqual(auth(undefined, { type: '@@tests/INIT' }));
  });
});

describe('network slice', () => {
  it('stores the current and saved networks', () => {
    const state = network(undefined, fetchNetworkData.fulfilled({ currentConnection: 'HomeNet', savedNetworks: ['HomeNet', 'Office'] }, 'req'));

    expect(state).toMatchObject({ currentConnection: 'HomeNet', savedNetworks: ['HomeNet', 'Office'], loading: false });
  });

  it.each([
    ['fetch', fetchNetworkData.rejected(null, 'req', undefined, 'boom')],
    ['add', addNetwork.rejected(null, 'req', { ssid: 'a', password: 'b' }, 'boom')],
    ['forget', forgetNetwork.rejected(null, 'req', 'a', 'boom')],
  ])('keeps the error of a failed %s', (_name, action) => {
    const loading = network(undefined, fetchNetworkData.pending('req'));

    expect(network(loading, action)).toMatchObject({ loading: false, error: 'boom' });
  });
});

describe('connectionMode slice', () => {
  it('marks the mode as loaded and normalises missing fields', () => {
    const state = connectionMode(undefined, fetchConnectionMode.fulfilled({ mode: 'ap' }, 'req'));

    expect(state).toEqual({ mode: 'ap', ssid: '', internet: false, ap_ssid: '', ap_password: '', loaded: true });
  });

  it('falls back to connecting when the mode is missing', () => {
    const state = connectionMode(undefined, fetchConnectionMode.fulfilled({ internet: true }, 'req'));

    expect(state).toMatchObject({ mode: 'connecting', internet: true, loaded: true });
  });

  it('stays unloaded when the request fails', () => {
    const state = connectionMode(undefined, fetchConnectionMode.rejected(null, 'req'));

    expect(state.loaded).toBe(false);
  });
});

describe('updates slice', () => {
  it('stores the latest release, status and history', () => {
    const release = makeRelease();
    const status = makeUpdateStatus({ phase: 'downloading', progress: 40 });
    const history = [makeUpdateHistoryEntry()];

    let state = updates(undefined, fetchLatestRelease.pending('req'));
    expect(state.loading).toBe(true);
    state = updates(state, fetchLatestRelease.fulfilled(release, 'req'));
    state = updates(state, fetchUpdateStatus.fulfilled(status, 'req'));
    state = updates(state, fetchUpdateHistory.fulfilled(history, 'req'));

    expect(state).toEqual({ latest_release: release, update_status: status, history, loading: false });
  });

  it('stops loading when the release lookup fails', () => {
    const loading = updates(undefined, fetchLatestRelease.pending('req'));

    expect(updates(loading, fetchLatestRelease.rejected(null, 'req')).loading).toBe(false);
  });
});

describe('services slice', () => {
  it('stores the services and selects one by id', () => {
    const list = healthyServices();
    const state = services(undefined, fetchServices.fulfilled(list, 'req'));
    const root = { ...rootState(), services: state };

    expect(state.services).toEqual(list);
    expect(selectService('display')(root)).toEqual(list[0]);
    expect(selectService('missing')(root)).toBeNull();
  });

  it('keeps the error of a failed fetch', () => {
    expect(services(undefined, fetchServices.rejected(null, 'req', undefined, 'boom'))).toMatchObject({ loading: false, error: 'boom' });
  });

  it('tracks which service is restarting', () => {
    const restarting = services(undefined, restartService.pending('req', 'display'));
    expect(restarting.restarting).toBe('display');

    expect(services(restarting, restartService.fulfilled('display', 'req', 'display')).restarting).toBeNull();
    expect(services(restarting, restartService.rejected(null, 'req', 'display')).restarting).toBeNull();
  });

  it('labels unknown services by id', () => {
    const state = services(undefined, fetchServices.fulfilled([makeService('custom')], 'req'));

    expect(state.services[0].label).toBe('custom');
  });
});

describe('frameInfo and displayStats slices', () => {
  it('store the fetched data', () => {
    const info = makeFrameInfo();
    const stats = makeDisplayStats();

    expect(frameInfo(undefined, fetchFrameInfos.fulfilled(info, 'req'))).toEqual({ frameInfo: info, loading: false });
    expect(displayStats(undefined, fetchDisplayStats.fulfilled(stats, 'req'))).toEqual({ displayStats: stats, loading: false });
  });

  it('display stats stop loading on failure', () => {
    const loading = displayStats(undefined, fetchDisplayStats.pending('req'));

    expect(displayStats(loading, fetchDisplayStats.rejected(null, 'req')).loading).toBe(false);
  });

  it('frame info stops loading on failure', () => {
    const loading = frameInfo(undefined, fetchFrameInfos.pending('req'));

    expect(frameInfo(loading, fetchFrameInfos.rejected(null, 'req')).loading).toBe(false);
  });
});
