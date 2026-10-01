import { afterEach, describe, expect, it, vi } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import { http, HttpResponse } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { card, findCard } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { advance, useBoardClock } from '@tests/helpers/time';
import { makeFrameInfo } from '@tests/fixtures';
import { apiUrl, failsWith, ok, server, spyOnRequests, withFrameInfo } from '@tests/mocks';

const openGeneral = () => renderRoute('/general/', { preloadedState: signedInState() });

describe('system info', () => {
  it('shows device, network and system metrics', async () => {
    server.use(
      ...withFrameInfo(
        makeFrameInfo({
          serial_number: 'SER-1',
          boot_slot: 'B',
          slot_trial: true,
          wifi_mode: 'ap',
          uptime_seconds: 90_061,
          ram_total_bytes: 512 * 1024 ** 2,
          ram_available_bytes: 256 * 1024 ** 2,
        }),
      ),
    );
    openGeneral();

    const device = await findCard('Gerät');
    expect(await within(device).findByText('SER-1')).toBeInTheDocument();
    expect(within(device).getByText('B (Trial, nicht bestätigt)')).toBeInTheDocument();
    expect(within(device).getByText('1d 1h 1m')).toBeInTheDocument();
    expect(within(card('Netzwerk')).getByText('Access Point')).toBeInTheDocument();
    expect(within(card('System')).getByText('256 MB / 512 MB (50%)')).toBeInTheDocument();
    expect(within(card('System')).getByText('45.2 °C')).toBeInTheDocument();
  });

  it('reports a failed load', async () => {
    server.use(...failsWith('get', api.getSystemInfoUrl()));
    openGeneral();

    expect(await screen.findAllByText('Abrufen der Frame-Informationen fehlgeschlagen')).not.toHaveLength(0);
  });
});

describe('device password', () => {
  const openDialog = async () => {
    const view = openGeneral();
    await view.user.click(await screen.findByRole('button', { name: 'Passwort ändern' }));
    return { ...view, dialog: await screen.findByRole('dialog') };
  };

  it('changes the password', async () => {
    const change = spyOnRequests('post', api.getChangePasswordUrl());
    server.use(change.handler);
    const { user, dialog } = await openDialog();

    await user.type(within(dialog).getByLabelText(/Aktuelles Passwort/), 'oldpassword');
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'newpassword');
    await user.click(within(dialog).getByRole('button', { name: 'Ändern' }));

    expect(await screen.findByText('Passwort erfolgreich geändert')).toBeInTheDocument();
    expect(await change.calls[0].json()).toEqual({ current_password: 'oldpassword', new_password: 'newpassword' });
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
  });

  it('needs a matching confirmation', async () => {
    const { user, dialog } = await openDialog();

    await user.type(within(dialog).getByLabelText(/Aktuelles Passwort/), 'oldpassword');
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'otherpassword');

    expect(within(dialog).getByText('Passwörter stimmen nicht überein')).toBeInTheDocument();
    expect(within(dialog).getByRole('button', { name: 'Ändern' })).toBeDisabled();
  });

  it('keeps the dialog open when the current password is wrong', async () => {
    server.use(...failsWith('post', api.getChangePasswordUrl(), 401, 'Aktuelles Passwort ungültig'));
    const { user, dialog } = await openDialog();

    await user.type(within(dialog).getByLabelText(/Aktuelles Passwort/), 'wrongpassword');
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'newpassword');
    await user.click(within(dialog).getByRole('button', { name: 'Ändern' }));

    expect(await screen.findByText('Aktuelles Passwort ungültig')).toBeInTheDocument();
    expect(screen.getByRole('dialog')).toBeInTheDocument();
  });
});

describe('power', () => {
  // A power command takes the board down; health answers again only after a reboot.
  const boardGoesDownOn = (path: string) => {
    const state = { healthy: true, commands: 0 };
    server.use(
      http.post(apiUrl(path), () => {
        state.commands += 1;
        state.healthy = false;
        return ok();
      }),
      http.get(apiUrl(api.getSystemHealthUrl()), () => (state.healthy ? ok({ running: true }) : HttpResponse.error())),
    );
    return state;
  };

  const openPowerMenu = async () => {
    const view = openGeneral();
    await view.user.click(await screen.findByRole('button', { name: 'Energieoptionen' }));
    return view;
  };

  afterEach(() => {
    vi.useRealTimers();
  });

  it('restarts the board and reloads once it is back', async () => {
    useBoardClock();
    const board = boardGoesDownOn(api.getRestartPiUrl());
    const { user, router, store } = await openPowerMenu();

    await user.click(await screen.findByRole('menuitem', { name: 'Neustarten' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Neustarten' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/'));
    expect(board.commands).toBe(1);
    expect(store.getState().loadingWall.message).toMatch(/wird neu gestartet/);

    await advance(3_000);
    board.healthy = true;
    await advance(3_000);

    await waitFor(() => expect(window.location.reload).toHaveBeenCalled());
  });

  it('shuts the board down and explains how to power it on', async () => {
    useBoardClock();
    const clear = spyOnRequests('post', api.getClearDisplayUrl());
    server.use(clear.handler);
    const board = boardGoesDownOn(api.getShutdownPiUrl());
    const { user, store } = await openPowerMenu();

    await user.click(await screen.findByRole('menuitem', { name: 'Herunterfahren' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Herunterfahren' }));

    await waitFor(() => expect(board.commands).toBe(1));
    expect(clear.calls).toHaveLength(1);
    expect(store.getState().loadingWall.message).toBe('Der Bilderrahmen wird heruntergefahren …');

    await advance(6_000);

    await waitFor(() => expect(store.getState().loadingWall.message).toMatch(/wurde heruntergefahren und ist ausgeschaltet/));
  });

  it('stays put when the board refuses to restart', async () => {
    server.use(...failsWith('post', api.getRestartPiUrl(), 409, 'Update läuft'));
    const { user, router, store } = await openPowerMenu();

    await user.click(await screen.findByRole('menuitem', { name: 'Neustarten' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Neustarten' }));

    expect(await screen.findByText('Neustartprozess fehlgeschlagen')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/general/');
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(false);
  });
});
