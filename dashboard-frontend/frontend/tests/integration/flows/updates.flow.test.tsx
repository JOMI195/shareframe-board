import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeRelease, makeUpdateHistoryEntry, makeUpdateStatus } from '@tests/fixtures';
import {
  failsWith,
  server,
  spyOnRequests,
  withRelease,
  withUpdateHistory,
  withUpdateStatus,
} from '@tests/mocks';

const openUpdates = () => renderRoute('/updates/', { preloadedState: signedInState() });

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

    expect(await screen.findByText('Suche nach neuster Version fehlgeschlagen')).toBeInTheDocument();
  });

  it('offers a newer version and installs it after confirmation', async () => {
    const perform = spyOnRequests('post', api.getPerformUpdateUrl());
    server.use(...withRelease(makeRelease({ version: '1.3.0', release_notes: 'Neue Bildeffekte' })), perform.handler);
    const { user, router, store } = openUpdates();

    expect(await screen.findByText('Neue Version verfügbar')).toBeInTheDocument();
    expect(screen.getByText('Neue Bildeffekte')).toBeInTheDocument();

    await user.click(screen.getByRole('button', { name: 'Jetzt installieren' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Installieren' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/'));
    expect(perform.calls).toHaveLength(1);
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(true);
  });

  it('does not offer an older release', async () => {
    server.use(...withRelease(makeRelease({ version: '1.1.0' })));
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

  it('explains a failed update', async () => {
    server.use(...withUpdateStatus(makeUpdateStatus({ phase: 'failed', error: 'Signatur ungültig' })));
    openUpdates();

    expect(await screen.findByText('Update fehlgeschlagen')).toBeInTheDocument();
    expect(screen.getByText('Signatur ungültig')).toBeInTheDocument();
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

    expect(await screen.findByText('Noch keine Updates durchgeführt.')).toBeInTheDocument();
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
