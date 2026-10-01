import { describe, expect, it } from 'vitest';
import { screen, waitFor } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { buildState, signedInState } from '@tests/helpers/preloadedState';
import { apMode, failsWith, server, signedOut, spyOnRequests } from '@tests/mocks';

const signedOutInApMode = () => server.use(...signedOut(), ...apMode({ ap_ssid: 'shareframe-1234', ap_password: 'hotspot123' }));

describe('offline WiFi setup', () => {
  it('sends a signed-out user to the setup page while the board hosts its hotspot', async () => {
    signedOutInApMode();
    const { router } = renderRoute('/network/', { preloadedState: buildState({ connectionMode: { mode: 'ap', loaded: true } }) });

    expect(await screen.findByText('WLAN einrichten', { selector: 'h6' })).toBeInTheDocument();
    await waitFor(() => expect(router.state.location.pathname).toBe('/setup'));
    expect(screen.getByText('AP-Modus aktiv')).toBeInTheDocument();
    expect(await screen.findByText(/http:\/\/192\.168\.4\.1/)).toBeInTheDocument();
  });

  // The protected route redirects before the first mode poll answers, and the auth
  // pages are exempt from the hotspot redirect.
  it('offers the setup from the sign-in page when the mode was not known yet', async () => {
    signedOutInApMode();
    const { user, router } = renderRoute('/network/', { preloadedState: buildState() });

    await user.click(await screen.findByRole('button', { name: 'WLAN einrichten' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/setup'));
  });

  it('leaves the password sign-in reachable', async () => {
    signedOutInApMode();
    const { router } = renderRoute('/auth/sign-in', { preloadedState: buildState() });

    expect(await screen.findByText('Willkommen bei deinem Bilderrahmen')).toBeInTheDocument();
    await screen.findByText(/die OTP-Anmeldung ist derzeit nicht möglich/);
    expect(router.state.location.pathname).toBe('/auth/sign-in');
  });

  it('does not redirect a signed-in user', async () => {
    server.use(...apMode());
    const { router } = renderRoute('/network/', { preloadedState: signedInState() });

    expect(await screen.findByText('AP-Modus aktiv')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe('/network/');
  });

  it('saves a WiFi network and explains what happens next', async () => {
    signedOutInApMode();
    const connect = spyOnRequests('post', api.getConnectionConnectUrl());
    server.use(connect.handler);
    const { user } = renderRoute('/setup', { preloadedState: buildState() });

    await user.type(await screen.findByLabelText('Netzwerkname (SSID)'), 'HomeNet');
    await user.type(screen.getByLabelText('Passwort'), 'homepass1');
    await user.click(screen.getByRole('button', { name: 'Verbinden' }));

    expect(await screen.findByText('WLAN gespeichert')).toBeInTheDocument();
    expect(screen.getByText(/sich mit „HomeNet“ zu verbinden/)).toBeInTheDocument();
    expect(await connect.calls[0].json()).toEqual({ ssid: 'HomeNet', password: 'homepass1' });
  });

  it('needs a password of at least eight characters', async () => {
    signedOutInApMode();
    const { user } = renderRoute('/setup', { preloadedState: buildState() });

    await user.type(await screen.findByLabelText('Netzwerkname (SSID)'), 'HomeNet');
    await user.type(screen.getByLabelText('Passwort'), 'short');

    expect(screen.getByRole('button', { name: 'Verbinden' })).toBeDisabled();
  });

  it('stays on the form when saving fails', async () => {
    signedOutInApMode();
    server.use(...failsWith('post', api.getConnectionConnectUrl()));
    const { user } = renderRoute('/setup', { preloadedState: buildState() });

    await user.type(await screen.findByLabelText('Netzwerkname (SSID)'), 'HomeNet');
    await user.type(screen.getByLabelText('Passwort'), 'homepass1');
    await user.click(screen.getByRole('button', { name: 'Verbinden' }));

    expect(await screen.findByText('Hinzufügen des Netzwerks fehlgeschlagen')).toBeInTheDocument();
    expect(screen.queryByText('WLAN gespeichert')).not.toBeInTheDocument();
  });

  it('links to the password sign-in', async () => {
    signedOutInApMode();
    const { user, router } = renderRoute('/setup', { preloadedState: buildState() });

    await user.click(await screen.findByRole('button', { name: 'Stattdessen mit Passwort anmelden' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/auth/sign-in'));
  });
});
