import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { card } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { buildState, signedInState } from '@tests/helpers/preloadedState';
import { apMode, failsWith, server, signedOut, spyOnRequests, withNetworks } from '@tests/mocks';

const HOTSPOT = { ap_ssid: 'shareframe-board-1234', ap_password: 'hotspot123' };

const signedOutOnHotspot = () => server.use(...signedOut(), ...apMode(HOTSPOT));

const openNetworkOnHotspot = async (saved = ['HomeNet'], over = {}) => {
  server.use(...apMode({ ...HOTSPOT, ...over }), ...withNetworks('', saved));
  const view = renderRoute('/network/', { preloadedState: signedInState() });
  await screen.findByText('Hotspot aktiv');
  return view;
};

describe('signing in on the hotspot', () => {
  it('sends a signed-out user to the password sign-in', async () => {
    signedOutOnHotspot();
    const { router } = renderRoute('/network/', { preloadedState: buildState() });

    expect(await screen.findByText(/stellt den Hotspot „shareframe-board-1234“ bereit/)).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/auth/sign-in');
    expect(screen.getByLabelText(/Passwort/)).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: 'Mit OTP anmelden' })).not.toBeInTheDocument();
  });

  it('opens the network settings after signing in', async () => {
    signedOutOnHotspot();
    const login = spyOnRequests('post', api.getLoginUrl());
    server.use(login.handler);
    const { user, router } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    await screen.findByText(/stellt den Hotspot/);
    await user.type(screen.getByLabelText(/Passwort/), 'geheim123');
    await user.click(screen.getByRole('button', { name: 'Anmelden' }));

    expect(await login.calls[0].json()).toEqual({ password: 'geheim123' });
    await waitFor(() => expect(router.state.location.pathname).toBe('/network/'));
  });

  it('resets the frame to factory defaults without signing in', async () => {
    signedOutOnHotspot();
    const reset = spyOnRequests('post', api.getFactoryResetUrl());
    server.use(reset.handler);
    const { user, store } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    await user.click(await screen.findByRole('button', { name: /Auf Werkseinstellungen zurücksetzen/ }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Zurücksetzen' }));

    await waitFor(() => expect(reset.calls).toHaveLength(1));
    expect(store.getState().loadingWall.message).toMatch(/auf Werkseinstellungen zurückgesetzt …/);
  });
});

describe('connecting from the hotspot', () => {
  it('only shows the hotspot card while the board hosts it', async () => {
    renderRoute('/network/', { preloadedState: signedInState() });

    await screen.findByText('Übersicht (WIFI)');
    expect(screen.queryByText('Hotspot aktiv')).not.toBeInTheDocument();
  });

  it('tells after saving a network how to switch over', async () => {
    const { user } = await openNetworkOnHotspot();

    await user.click(screen.getByRole('button', { name: 'Netzwerk hinzufügen' }));
    const dialog = await screen.findByRole('dialog');
    await user.type(within(dialog).getByLabelText(/Netzwerkname/), 'Garden');
    await user.type(within(dialog).getByLabelText(/^Passwort/), 'gardenpass');
    await user.click(within(dialog).getByRole('button', { name: 'Hinzufügen' }));

    expect(await screen.findByText(/mit „Jetzt verbinden“ wechselt der Rahmen ins WLAN/)).toBeInTheDocument();
  });

  it('asks the board to try the saved networks and explains where to find it', async () => {
    const apply = spyOnRequests('post', api.getConnectionApplyUrl());
    server.use(apply.handler);
    const { user } = await openNetworkOnHotspot();

    await user.click(within(card('Hotspot aktiv')).getByRole('button', { name: 'Jetzt verbinden' }));
    const dialog = await screen.findByRole('dialog');
    expect(within(dialog).getByText('http://shareframe-board-1234.local')).toBeInTheDocument();
    await user.click(within(dialog).getByRole('button', { name: 'Verbinden' }));

    await waitFor(() => expect(apply.calls).toHaveLength(1));
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
    const hotspot = card('Hotspot aktiv');
    expect(await within(hotspot).findByText(/Der Rahmen verbindet sich/)).toBeInTheDocument();
    expect(within(hotspot).getByRole('button', { name: 'Jetzt verbinden' })).toBeDisabled();
  });

  it('reports a failed request', async () => {
    server.use(...failsWith('post', api.getConnectionApplyUrl(), 409, 'Not in access point mode'));
    const { user } = await openNetworkOnHotspot();

    await user.click(within(card('Hotspot aktiv')).getByRole('button', { name: 'Jetzt verbinden' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Verbinden' }));

    expect(await screen.findByText('Verbinden fehlgeschlagen')).toBeInTheDocument();
    expect(screen.getByRole('dialog')).toBeInTheDocument();
    expect(screen.queryByText(/Der Rahmen verbindet sich/)).not.toBeInTheDocument();
  });

  it('says when the last attempt failed', async () => {
    await openNetworkOnHotspot(['HomeNet'], { last_failed_secs_ago: 180 });

    expect(await within(card('Hotspot aktiv')).findByText(/Letzter Verbindungsversuch vor 3 Minuten fehlgeschlagen/))
      .toBeInTheDocument();
  });

  it('needs a saved network first', async () => {
    await openNetworkOnHotspot([]);

    expect(within(card('Hotspot aktiv')).getByRole('button', { name: 'Jetzt verbinden' })).toBeDisabled();
  });
});
