import { describe, expect, it } from 'vitest';
import * as api from '@/assets/endpoints/api/frame';
import * as app from '@/assets/endpoints/app/appEndpoints';
import * as auth from '@/assets/endpoints/app/authEndpoints';

// The paths are the contract with the C++ dashboard server.
describe('API endpoints', () => {
  it.each([
    [api.getAuthStatusUrl(), '/api/auth/status'],
    [api.getLoginUrl(), '/api/auth/login'],
    [api.getLogoutUrl(), '/api/auth/logout'],
    [api.getChangePasswordUrl(), '/api/auth/change-password'],
    [api.getConnectionStatusUrl(), '/api/connection/status'],
    [api.getConnectionSavedNetworksUrl(), '/api/connection/saved-networks'],
    [api.getConnectionConnectUrl(), '/api/connection/connect'],
    [api.getConnectionForgetUrl(), '/api/connection/forget'],
    [api.getConnectionModeUrl(), '/api/connection/mode'],
    [api.getApPasswordUrl(), '/api/connection/ap-password'],
    [api.getSlideshowUrl(), '/api/frame/slideshow'],
    [api.getSlideshowStatusUrl(), '/api/frame/slideshow/status'],
    [api.getSlideshowIntervalUrl(), '/api/frame/slideshow/interval'],
    [api.getSlideshowNightModeUrl(), '/api/frame/slideshow/night-mode'],
    [api.getSkipSlideshowImageUrl(), '/api/frame/slideshow/skip'],
    [api.getClearDisplayUrl(), '/api/frame/clear'],
    [api.getDisplayStatsUrl(), '/api/frame/display/stats'],
    [api.getServicesUrl(), '/api/services'],
    [api.getServiceRestartUrl(), '/api/services/restart'],
    [api.getSystemInfoUrl(), '/api/system/info'],
    [api.getSystemHealthUrl(), '/api/system/health'],
    [api.getRestartPiUrl(), '/api/system/restart'],
    [api.getShutdownPiUrl(), '/api/system/shutdown'],
    [api.getFrameLogsUrl(), '/api/system/logs'],
    [api.getLatestReleaseUrl(), '/api/system/updates/latest'],
    [api.getPerformUpdateUrl(), '/api/system/updates/perform-update'],
    [api.getUpdateStatusUrl(), '/api/system/updates/status'],
    [api.getUpdateHistoryUrl(), '/api/system/updates/history'],
  ])('%s', (actual, expected) => {
    expect(actual).toBe(expected);
  });
});

describe('app routes', () => {
  it('nests the auth pages under /auth', () => {
    expect(auth.getAuthenticationUrl() + auth.getSignInUrl()).toBe('/auth/sign-in');
    expect(auth.getAuthenticationUrl() + auth.getSignOutUrl()).toBe('/auth/sign-out');
  });

  it('builds service detail routes from the id', () => {
    expect(app.getServiceDetailUrl('display')).toBe('services/display');
  });
});
