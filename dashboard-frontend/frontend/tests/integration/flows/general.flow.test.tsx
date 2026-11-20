import { afterEach, describe, expect, it, vi } from 'vitest';
import { act, screen, waitFor, within } from '@testing-library/react';
import { http, HttpResponse } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { card, findCard } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { advance, useBoardClock } from '@tests/helpers/time';
import { makeFrameInfo } from '@tests/fixtures';
import { apiUrl, failsWith, ok, server, spyOnRequests, withConnectionMode, withFrameInfo } from '@tests/mocks';

const openGeneral = (expert = false) =>
  renderRoute('/general/', { preloadedState: signedInState({ navigation: { advancedMode: expert } }) });

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
    openGeneral(true);

    const device = await findCard('Gerät');
    expect(await within(device).findByText('SER-1')).toBeInTheDocument();
    expect(within(device).getByText('B (Trial, nicht bestätigt)')).toBeInTheDocument();
    expect(within(device).getByText('1d 1h 1m')).toBeInTheDocument();
    expect(within(card('Netzwerk')).getByText('Hotspot')).toBeInTheDocument();
    expect(within(card('System')).getByText('256 MB / 512 MB (50%)')).toBeInTheDocument();
    expect(within(card('System')).getByText('45.2 °C')).toBeInTheDocument();
  });

  it('shows network and system metrics only in expert mode', async () => {
    const { user, store } = openGeneral();

    expect(await findCard('Gerät')).toBeInTheDocument();
    expect(screen.queryByRole('heading', { level: 2, name: 'Netzwerk' })).not.toBeInTheDocument();
    expect(screen.queryByRole('heading', { level: 2, name: 'System' })).not.toBeInTheDocument();

    await user.click(screen.getByRole('switch', { name: 'Expertenmodus' }));

    expect(store.getState().navigation.advancedMode).toBe(true);
    expect(await findCard('Netzwerk')).toBeInTheDocument();
    expect(card('System')).toBeInTheDocument();
  });

  it('reports a failed load', async () => {
    server.use(...failsWith('get', api.getSystemInfoUrl()));
    openGeneral();

    expect(await screen.findAllByText('Systeminfos konnten nicht geladen werden')).not.toHaveLength(0);
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
    await user.click(within(dialog).getByRole('button', { name: 'Passwort ändern' }));

    expect(await screen.findByText('Passwort geändert')).toBeInTheDocument();
    expect(await change.calls[0].json()).toEqual({ current_password: 'oldpassword', new_password: 'newpassword' });
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
  });

  it('asks no current password after a sign-in with a code', async () => {
    const change = spyOnRequests('post', api.getChangePasswordUrl());
    server.use(change.handler);
    const { user } = renderRoute('/general/', { preloadedState: signedInState({ auth: { passwordChangeNeedsCurrent: false } }) });
    await user.click(await screen.findByRole('button', { name: 'Passwort ändern' }));
    const dialog = await screen.findByRole('dialog');

    expect(within(dialog).queryByLabelText(/Aktuelles Passwort/)).not.toBeInTheDocument();
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'newpassword');
    await user.click(within(dialog).getByRole('button', { name: 'Passwort ändern' }));

    await waitFor(() => expect(change.calls).toHaveLength(1));
    expect(await change.calls[0].json()).toEqual({ new_password: 'newpassword' });
  });

  it('needs a matching confirmation', async () => {
    const { user, dialog } = await openDialog();

    await user.type(within(dialog).getByLabelText(/Aktuelles Passwort/), 'oldpassword');
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'otherpassword');

    expect(within(dialog).getByText('Passwörter stimmen nicht überein')).toBeInTheDocument();
    expect(within(dialog).getByRole('button', { name: 'Passwort ändern' })).toBeDisabled();
  });

  it('keeps the dialog open when the current password is wrong', async () => {
    server.use(...failsWith('post', api.getChangePasswordUrl(), 401, 'Aktuelles Passwort ungültig'));
    const { user, dialog } = await openDialog();

    await user.type(within(dialog).getByLabelText(/Aktuelles Passwort/), 'wrongpassword');
    await user.type(within(dialog).getByLabelText(/^Neues Passwort(?! bestätigen)/), 'newpassword');
    await user.type(within(dialog).getByLabelText(/Neues Passwort bestätigen/), 'newpassword');
    await user.click(within(dialog).getByRole('button', { name: 'Passwort ändern' }));

    expect(await screen.findByText('Das aktuelle Passwort stimmt nicht.')).toBeInTheDocument();
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

    await user.click(await screen.findByRole('menuitem', { name: 'Neu starten' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Neu starten' }));

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

    await waitFor(() => expect(store.getState().loadingWall.message).toMatch(/Der Bilderrahmen ist ausgeschaltet/));
  });

  it('stays put when the board refuses to restart', async () => {
    server.use(...failsWith('post', api.getRestartPiUrl(), 409, 'Update läuft'));
    const { user, router, store } = await openPowerMenu();

    await user.click(await screen.findByRole('menuitem', { name: 'Neu starten' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Neu starten' }));

    expect(await screen.findByText('Bilderrahmen konnte nicht neu gestartet werden')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/general/');
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(false);
  });

  const openFactoryReset = async () => {
    const view = openGeneral();
    await view.user.click(await screen.findByRole('button', { name: 'Zurücksetzen' }));
    await screen.findByRole('heading', { level: 2, name: 'Auf Werkseinstellungen zurücksetzen' });
    return view;
  };

  // Asks the frame for its code, then types it.
  const enterCode = async (user: ReturnType<typeof openGeneral>['user'], code = '731204') => {
    await user.click(screen.getByRole('button', { name: 'Code auf dem Bilderrahmen anzeigen' }));
    await user.type(await screen.findByLabelText('Code'), code);
    await user.click(screen.getByRole('button', { name: 'Endgültig zurücksetzen' }));
  };

  it('opens the reset as its own page', async () => {
    const { router } = await openFactoryReset();

    expect(router.state.location.pathname).toBe('/general/factory-reset/');
    expect(screen.queryByRole('dialog')).not.toBeInTheDocument();
  });

  it('resets to factory defaults and explains how to set the frame up again', async () => {
    useBoardClock();
    const board = boardGoesDownOn(api.getFactoryResetUrl());
    const { user, router, store } = await openFactoryReset();

    await enterCode(user);

    await waitFor(() => expect(router.state.location.pathname).toBe('/'));
    expect(board.commands).toBe(1);
    expect(store.getState().loadingWall.message).toMatch(/auf Werkseinstellungen zurückgesetzt …/);

    await advance(6_000);

    await waitFor(() => expect(store.getState().loadingWall.message).toMatch(/startet im Einrichtungsmodus/));
    expect(window.location.reload).not.toHaveBeenCalled();
  });

  it('asks to turn the display on first while it is off', async () => {
    const code = spyOnRequests('post', api.getFactoryResetCodeUrl());
    server.use(...withConnectionMode({ safe_mode: 'remote' }), code.handler);
    await openFactoryReset();

    expect(await screen.findByText(/Schalte sie zuerst über „Anzeige wieder einschalten“/)).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Code auf dem Bilderrahmen anzeigen' })).toBeDisabled();
    expect(code.calls).toHaveLength(0);
  });

  it('goes back to the system page without resetting', async () => {
    const reset = spyOnRequests('post', api.getFactoryResetUrl());
    server.use(reset.handler);
    const { user, router } = await openFactoryReset();

    await user.click(screen.getByRole('button', { name: 'Abbrechen' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/general/'));
    expect(reset.calls).toHaveLength(0);
  });

  it('stays put when the board refuses to reset', async () => {
    server.use(...failsWith('post', api.getFactoryResetUrl(), 500, 'Factory reset failed'));
    const { user, router, store } = await openFactoryReset();

    await enterCode(user);

    expect(await screen.findByText('Bilderrahmen konnte nicht zurückgesetzt werden')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/general/factory-reset/');
    expect(store.getState().loadingWall.isLoadingWallVisible).toBe(false);
  });
});

describe('factory reset by the code on the panel', () => {
  const openCodeStep = async () => {
    const view = openGeneral();
    await view.user.click(await screen.findByRole('button', { name: 'Zurücksetzen' }));
    await view.user.click(await screen.findByRole('button', { name: 'Code auf dem Bilderrahmen anzeigen' }));
    await screen.findByLabelText('Code');
    return view;
  };

  it('says when the code will be on the frame', async () => {
    server.use(http.post(apiUrl(api.getFactoryResetCodeUrl()), () => ok({ expires_in_secs: 300, shows_in_secs: 95 })));
    await openCodeStep();
    expect(screen.getByText(/erscheint in etwa 1:3\d auf dem Bildschirm/)).toBeInTheDocument();
  });

  it('stays on the code with the tries left after a wrong one', async () => {
    server.use(http.post(apiUrl(api.getFactoryResetUrl()), () =>
      HttpResponse.json({ success: false, message: 'Wrong code', data: { tries_left: 3 } }, { status: 403 })));
    const { user, router } = await openCodeStep();

    await user.type(screen.getByLabelText('Code'), '000000');
    await user.click(screen.getByRole('button', { name: 'Endgültig zurücksetzen' }));

    expect(await screen.findByText('Der Code stimmt nicht. Noch 3 Versuche.')).toBeInTheDocument();
    expect(screen.getByLabelText('Code')).toHaveValue('');
    expect(router.state.location.pathname).toBe('/general/factory-reset/');
  });

  it('takes the code back from the frame on Abbrechen', async () => {
    const cancel = spyOnRequests('post', api.getFactoryResetCancelUrl());
    server.use(cancel.handler);
    const { user, router } = await openCodeStep();
    expect(screen.getByText('Noch 5 Minuten gültig')).toBeInTheDocument();

    await user.click(screen.getByRole('button', { name: 'Abbrechen' }));

    await waitFor(() => expect(cancel.calls).toHaveLength(1));
    expect(router.state.location.pathname).toBe('/general/');
  });

  // The menu or the back button leave the page as well; the code must not stay on the frame.
  it('takes the code back from the frame when the page is left any other way', async () => {
    const cancel = spyOnRequests('post', api.getFactoryResetCancelUrl());
    server.use(cancel.handler);
    const { router } = await openCodeStep();

    await act(() => router.navigate('/'));

    await waitFor(() => expect(cancel.calls).toHaveLength(1));
  });

  it('explains why no code can appear', async () => {
    server.use(...failsWith('post', api.getFactoryResetCodeUrl(), 409, 'An update is running'));
    const { user } = openGeneral();
    await user.click(await screen.findByRole('button', { name: 'Zurücksetzen' }));
    await user.click(await screen.findByRole('button', { name: 'Code auf dem Bilderrahmen anzeigen' }));

    expect(await screen.findByText('Während ein Update installiert wird, geht das nicht.')).toBeInTheDocument();
    expect(screen.queryByLabelText('Code')).not.toBeInTheDocument();
  });
});

describe('diagnostics', () => {
  it('sends the support package and says so; the button waits for the board', async () => {
    let release!: () => void;
    const sent = new Promise<void>((resolve) => { release = resolve; });
    server.use(http.post(apiUrl(api.getSendDiagnosticsUrl()), async () => { await sent; return ok(); }));
    openGeneral();

    const button = await screen.findByRole('button', { name: 'Diagnose senden' });
    button.click();
    expect(await screen.findByRole('button', { name: 'Wird gesendet …' })).toBeDisabled();
    release();
    expect(await screen.findByText('Diagnose an den Support gesendet')).toBeInTheDocument();
  });

  it('a failed upload says it failed', async () => {
    server.use(...failsWith('post', api.getSendDiagnosticsUrl(), 502, 'Upload fehlgeschlagen'));
    openGeneral();
    (await screen.findByRole('button', { name: 'Diagnose senden' })).click();
    expect(await screen.findByText('Diagnose konnte nicht gesendet werden')).toBeInTheDocument();
  });
});
