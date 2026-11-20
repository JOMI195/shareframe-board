import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { card, findCard } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { buildState, signedInState } from '@tests/helpers/preloadedState';
import { apMode, failsWith, server, signedOut, spyOnRequests, withNetworks } from '@tests/mocks';

// Like the prod frames: the hotspot keeps the plain name, the board's own name has its suffix.
const HOTSPOT = { ap_ssid: 'shareframe-board', ap_password: 'hotspot123', hostname: 'shareframe-board-1234' };

const signedOutOnHotspot = () => server.use(...signedOut(), ...apMode(HOTSPOT));

const openNetworkOnHotspot = async (saved = ['HomeNet'], over = {}) => {
  server.use(...apMode({ ...HOTSPOT, ...over }), ...withNetworks('', saved));
  const view = renderRoute('/network/', { preloadedState: signedInState() });
  await screen.findByRole('heading', { level: 2, name: 'WLAN einrichten' });
  return view;
};

describe('signing in on the hotspot', () => {
  it('asks a signed-out user for the setup code from the panel', async () => {
    signedOutOnHotspot();
    const { router } = renderRoute('/network/', { preloadedState: buildState() });

    expect(await screen.findByText(/stellt den Hotspot „shareframe-board“ bereit/)).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/auth/sign-in');
    expect(screen.getByText(/Er steht auf dem Bildschirm des Bilderrahmens/)).toBeInTheDocument();
    expect(screen.getByLabelText(/^Einrichtungscode/, { selector: 'input' })).toHaveAttribute('type', 'text');
    expect(screen.queryByRole('button', { name: 'Mit Einmal-Code anmelden' })).not.toBeInTheDocument();
  });

  it('opens the network settings after signing in with the setup code', async () => {
    signedOutOnHotspot();
    const login = spyOnRequests('post', api.getLoginUrl());
    server.use(login.handler);
    const { user, router } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    await screen.findByText(/stellt den Hotspot/);
    await user.type(screen.getByLabelText(/^Einrichtungscode/, { selector: 'input' }), '482 913');
    await user.click(screen.getByRole('button', { name: 'Anmelden' }));

    expect(await login.calls[0].json()).toEqual({ setup_code: '482913' });
    await waitFor(() => expect(router.state.location.pathname).toBe('/network/'));
  });

  it('still takes the device password', async () => {
    signedOutOnHotspot();
    const login = spyOnRequests('post', api.getLoginUrl());
    server.use(login.handler);
    const { user } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    await user.click(await screen.findByRole('button', { name: 'Mit Geräte-Passwort anmelden' }));
    await user.type(screen.getByLabelText(/^Passwort/, { selector: 'input' }), 'geheim123');
    await user.click(screen.getByRole('button', { name: 'Anmelden' }));

    expect(await login.calls[0].json()).toEqual({ password: 'geheim123' });
  });

  it('resets a frame whose password is forgotten with the code from its panel', async () => {
    signedOutOnHotspot();
    const reset = spyOnRequests('post', api.getFactoryResetUrl());
    server.use(reset.handler);
    const { user, store } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    await user.click(await screen.findByRole('link', { name: 'Passwort vergessen?' }));
    expect(await screen.findByRole('heading', { level: 1, name: 'Passwort vergessen?' })).toBeInTheDocument();
    expect(screen.getByText(/mit dem Einrichtungscode .* So bleibt alles erhalten/)).toBeInTheDocument();
    await user.click(screen.getByRole('button', { name: 'Code auf dem Bilderrahmen anzeigen' }));

    await user.type(await screen.findByLabelText('Code'), '731 204');
    await user.click(screen.getByRole('button', { name: 'Endgültig zurücksetzen' }));

    await waitFor(() => expect(reset.calls).toHaveLength(1));
    expect(await reset.calls[0].json()).toEqual({ code: '731204' });
    expect(store.getState().loadingWall.message).toMatch(/auf Werkseinstellungen zurückgesetzt …/);
  });
});

describe('connecting from the hotspot', () => {
  it('only shows the hotspot card while the board hosts it', async () => {
    renderRoute('/network/', { preloadedState: signedInState() });

    await screen.findByRole('heading', { level: 2, name: 'WLAN' });
    expect(screen.queryByRole('heading', { name: 'WLAN einrichten' })).not.toBeInTheDocument();
  });

  it('saves a network and switches over to it in one step', async () => {
    const connect = spyOnRequests('post', api.getConnectionConnectUrl());
    const apply = spyOnRequests('post', api.getConnectionApplyUrl());
    server.use(connect.handler, apply.handler);
    const { user, router } = await openNetworkOnHotspot([]);

    await user.click(screen.getByRole('button', { name: 'WLAN hinzufügen' }));
    const dialog = await findCard('WLAN hinzufügen');
    expect(within(dialog).getByText(/http:\/\/shareframe-board-1234\.local/)).toBeInTheDocument();
    await user.type(within(dialog).getByLabelText(/Netzwerkname/), 'Garden');
    await user.type(within(dialog).getByLabelText(/^Passwort$/), 'gardenpass');
    await user.click(within(dialog).getByRole('button', { name: 'Speichern & verbinden' }));

    await waitFor(() => expect(apply.calls).toHaveLength(1));
    expect(connect.calls).toHaveLength(1);
    await waitFor(() => expect(router.state.location.pathname).toBe('/network/'));
    expect(await within(await findCard('WLAN einrichten')).findByText(/Der Rahmen verbindet sich/)).toBeInTheDocument();
  });

  it('asks the board to try the saved networks and explains where to find it', async () => {
    const apply = spyOnRequests('post', api.getConnectionApplyUrl());
    server.use(apply.handler);
    const { user } = await openNetworkOnHotspot();

    await user.click(within(card('WLAN einrichten')).getByRole('button', { name: 'Jetzt verbinden' }));
    const dialog = await screen.findByRole('dialog');
    // The board's own name, not the hotspot's: they differ on the prod frames.
    expect(within(dialog).getByText('http://shareframe-board-1234.local')).toBeInTheDocument();
    await user.click(within(dialog).getByRole('button', { name: 'Verbinden' }));

    await waitFor(() => expect(apply.calls).toHaveLength(1));
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
    const hotspot = card('WLAN einrichten');
    expect(await within(hotspot).findByText(/Der Rahmen verbindet sich/)).toBeInTheDocument();
    expect(within(hotspot).getByRole('button', { name: 'Jetzt verbinden' })).toBeDisabled();
  });

  it('reports a failed request', async () => {
    server.use(...failsWith('post', api.getConnectionApplyUrl(), 409, 'Not in access point mode'));
    const { user } = await openNetworkOnHotspot();

    await user.click(within(card('WLAN einrichten')).getByRole('button', { name: 'Jetzt verbinden' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Verbinden' }));

    expect(await screen.findByText('Das geht nur, solange der Bilderrahmen seinen Hotspot bereitstellt.')).toBeInTheDocument();
    expect(screen.getByRole('dialog')).toBeInTheDocument();
    expect(screen.queryByText(/Der Rahmen verbindet sich/)).not.toBeInTheDocument();
  });

  it('says when the last attempt failed', async () => {
    await openNetworkOnHotspot(['HomeNet'], { last_failed_secs_ago: 180 });

    expect(await within(card('WLAN einrichten')).findByText(/Vor 3 Minuten war keines der gespeicherten/))
      .toBeInTheDocument();
  });

  it('names why the last attempt failed', async () => {
    await openNetworkOnHotspot(['HomeNet'], { last_failed_secs_ago: 180, last_failed_reason: 'wrong_password' });

    expect(await within(card('WLAN einrichten')).findByText(/Das WLAN-Passwort stimmt nicht/)).toBeInTheDocument();
  });

  it('needs a saved network first', async () => {
    await openNetworkOnHotspot([]);

    expect(within(card('WLAN einrichten')).getByRole('button', { name: 'Jetzt verbinden' })).toBeDisabled();
  });
});
