import { afterEach, describe, expect, it, vi } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { advance, useBoardClock } from '@tests/helpers/time';
import { makeRelease, makeUpdateHistoryEntry, makeUpdateStatus } from '@tests/fixtures';
import {
  apMode,
  failsWith,
  server,
  spyOnRequests,
  withRelease,
  withUpdateHistory,
  withConnectionMode,
  withUpdateStatus,
} from '@tests/mocks';

const openUpdates = () => renderRoute('/updates/', { preloadedState: signedInState() });

afterEach(() => {
  vi.useRealTimers();
});

describe('updates', () => {
  it('shows the installed version and that it is up to date', async () => {
    openUpdates();

    expect(await screen.findByText('1.2.0')).toBeInTheDocument();
    expect(await screen.findByText('Keine Updates verfügbar')).toBeInTheDocument();
  });

  it('checks for updates on request', async () => {
    const latest = spyOnRequests('get', api.getLatestReleaseUrl(), () =>
      Response.json({ success: true, message: '', data: null }),
    );
    server.use(latest.handler);
    const { user } = openUpdates();
    await waitFor(() => expect(latest.calls).toHaveLength(1));

    await user.click(await screen.findByRole('button', { name: 'Auf Updates prüfen' }));

    await waitFor(() => expect(latest.calls).toHaveLength(2));
  });

  it('reports a failed update lookup', async () => {
    server.use(...failsWith('get', api.getLatestReleaseUrl()));
    openUpdates();

    expect(await screen.findByText('Updates konnten nicht gesucht werden')).toBeInTheDocument();
  });

  it.each([
    ['on the hotspot', apMode(), { mode: 'ap', ssid: '', internet: false } as const],
    ['without internet', withConnectionMode({ internet: false }), { internet: false }],
  ])('does not look for updates %s', async (_, connection, preloaded) => {
    const latest = spyOnRequests('get', api.getLatestReleaseUrl());
    server.use(...connection, latest.handler);
    renderRoute('/updates/', { preloadedState: signedInState({ connectionMode: preloaded }) });

    expect(await screen.findByText('Ohne Internet kann nicht nach Updates gesucht werden.')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Auf Updates prüfen' })).toBeDisabled();
    expect(latest.calls).toHaveLength(0);
    expect(screen.queryByText('Updates konnten nicht gesucht werden')).not.toBeInTheDocument();
  });

  it('looks for updates once the frame is online', async () => {
    const latest = spyOnRequests('get', api.getLatestReleaseUrl(), () =>
      Response.json({ success: true, message: '', data: null }),
    );
    useBoardClock();
    server.use(...apMode(), latest.handler);
    renderRoute('/updates/', { preloadedState: signedInState({ connectionMode: { mode: 'ap', ssid: '', internet: false } }) });
    expect(await screen.findByText('Keine Internetverbindung')).toBeInTheDocument();

    server.use(...withConnectionMode({}));
    await advance(10_000);
    await waitFor(() => expect(latest.calls).toHaveLength(1));
    expect(await screen.findByText('Keine Updates verfügbar')).toBeInTheDocument();
  });

  it('lists the bullet points of the release notes', async () => {
    server.use(...withRelease(makeRelease({ version: '1.3.0', release_notes: '- Neue Bildeffekte\n\n- Bugfixes' })));
    openUpdates();

    const items = await screen.findAllByRole('listitem');
    expect(items.map((item) => item.textContent)).toEqual(['Neue Bildeffekte', 'Bugfixes']);
  });

  it('hides the notes section for a release without notes', async () => {
    server.use(...withRelease(makeRelease({ version: '1.3.0', release_notes: '' })));
    openUpdates();

    expect(await screen.findByText('Neue Version verfügbar')).toBeInTheDocument();
    expect(screen.queryByText('Umfang')).not.toBeInTheDocument();
  });

  it('offers a newer version and installs it after confirmation', async () => {
    const perform = spyOnRequests('post', api.getPerformUpdateUrl());
    server.use(...withRelease(makeRelease({ version: '1.3.0', release_notes: 'Neue Bildeffekte' })), perform.handler);
    const { user, router, store } = openUpdates();

    expect(await screen.findByText('Neue Version verfügbar')).toBeInTheDocument();
    expect(screen.getByText('Neue Bildeffekte')).toBeInTheDocument();

    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'downloading', progress: 10, target_version: '1.3.0' })));
    await user.click(screen.getByRole('button', { name: 'Jetzt installieren' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Installieren' }));

    // The page follows the download and the install; only the restart hides it.
    await waitFor(() => expect(perform.calls).toHaveLength(1));
    expect(await screen.findByRole('progressbar', { name: 'Update-Fortschritt' })).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/updates/');
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(false);
  });

  it('covers the page while the frame restarts into the new version', async () => {
    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'awaiting-reboot', target_version: '1.3.0' })));
    const { store } = openUpdates();

    await waitFor(() => expect(store.getState().loadingWall.isLoadingWallVisible).toBe(true));
    expect(store.getState().loadingWall.message).toMatch(/startet jetzt mit der neuen Version neu/);
    expect(store.getState().loadingWall.hideAfter! - Date.now()).toBeGreaterThan(5 * 60_000);
  });

  it('does not offer a release the board reports as not newer', async () => {
    server.use(...withRelease(makeRelease({ version: '1.1.0', is_newer: false })));
    openUpdates();

    expect(await screen.findByText('Keine Updates verfügbar')).toBeInTheDocument();
  });

  it('shows the progress of a running download', async () => {
    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'downloading', progress: 40, target_version: '1.3.0' })));
    openUpdates();

    expect(await screen.findByText('Lade Update herunter (1.3.0)')).toBeInTheDocument();
    const bars = screen.getAllByRole('progressbar').filter((bar) => bar.hasAttribute('aria-valuenow'));
    expect(bars.map((bar) => bar.getAttribute('aria-valuenow'))).toEqual(['40']);
  });

  it('blocks a new install while the last one awaits confirmation', async () => {
    server.use(
      ...withRelease(makeRelease({ version: '1.3.0' })),
      ...withUpdateStatus(makeUpdateStatus({ booted_slot: 'B', pending_slot: 'B', committed: false })),
    );
    openUpdates();

    expect(await screen.findByText('Update wird bestätigt')).toBeInTheDocument();
    expect(await screen.findByRole('button', { name: 'Jetzt installieren' })).toBeDisabled();
  });

  it('explains a failed update in German and keeps the board\'s reason for support', async () => {
    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'failed', error: 'Download failed: HTTP 404' })));
    openUpdates();

    expect(await screen.findByText('Update fehlgeschlagen')).toBeInTheDocument();
    expect(screen.getByText(/^Das Update konnte nicht heruntergeladen werden\./)).toBeInTheDocument();
    expect(screen.getByText('Details: Download failed: HTTP 404')).toBeInTheDocument();
  });

  it('explains an unknown failure without hiding it', async () => {
    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'failed', error: 'something new' })));
    openUpdates();

    expect(await screen.findByText(/^Das Update ist fehlgeschlagen\./)).toBeInTheDocument();
    expect(screen.getByText('Details: something new')).toBeInTheDocument();
  });

  it('leads to the update history from the overview', async () => {
    const { user, router } = openUpdates();

    await user.click(await screen.findByRole('button', { name: 'Update-Verlauf' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/update-history/'));
  });

  it('explains a rollback', async () => {
    server.use(
      ...withUpdateStatus(
        makeUpdateStatus({ last_result: makeUpdateHistoryEntry({ from_version: '1.2.0', to_version: '1.3.0', result: 'rolled-back' }) }),
      ),
    );
    openUpdates();

    expect(await screen.findByText('Letztes Update zurückgerollt')).toBeInTheDocument();
    expect(screen.getByText(/Version 1.3.0 konnte nicht gestartet werden/)).toBeInTheDocument();
  });
});

describe('update history', () => {
  const openHistory = () => renderRoute('/update-history/', { preloadedState: signedInState() });

  it('says so when nothing was installed yet', async () => {
    openHistory();

    expect(await screen.findByText('Noch keine Updates installiert.')).toBeInTheDocument();
  });

  it('lists each update with its outcome', async () => {
    server.use(
      ...withUpdateHistory([
        makeUpdateHistoryEntry({ from_version: '1.0.0', to_version: '1.1.0' }),
        makeUpdateHistoryEntry({ from_version: '1.1.0', to_version: '1.2.0', result: 'rolled-back', error: 'Healthcheck fehlgeschlagen' }),
      ]),
    );
    const { user } = openHistory();

    expect(await screen.findByText('1.0.0 → 1.1.0')).toBeInTheDocument();
    expect(screen.getByText('Erfolgreich')).toBeInTheDocument();
    expect(screen.getByText('Zurückgerollt')).toBeInTheDocument();

    await user.click(screen.getByText('1.1.0 → 1.2.0'));
    expect(await screen.findByText('Healthcheck fehlgeschlagen')).toBeVisible();
  });

  it('reports a failed load', async () => {
    server.use(...failsWith('get', api.getUpdateHistoryUrl()));
    openHistory();

    expect(await screen.findByText('Update-Verlauf konnte nicht geladen werden')).toBeInTheDocument();
  });
});
