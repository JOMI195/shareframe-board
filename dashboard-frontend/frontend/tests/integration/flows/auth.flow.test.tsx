import { describe, expect, it } from 'vitest';
import { screen, waitFor } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { setupStore } from '@/store/setupStore';
import { renderRoute } from '@tests/helpers/renderRoute';
import { buildState, signedInState } from '@tests/helpers/preloadedState';
import { boardUnreachable, failsWith, server, spyOnRequests, withConnectionMode } from '@tests/mocks';

const SIGN_IN = '/auth/sign-in';

const openSignIn = async () => {
  const view = renderRoute(SIGN_IN, { preloadedState: buildState() });
  await screen.findByText('Willkommen bei deinem Bilderrahmen');
  return view;
};

describe('protected routes', () => {
  it('sends a signed-out user to the sign-in page', async () => {
    const { router } = renderRoute('/network/', { preloadedState: buildState() });

    expect(await screen.findByText('Willkommen bei deinem Bilderrahmen')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe(SIGN_IN);
  });

  it('lets a signed-in user through', async () => {
    renderRoute('/', { preloadedState: signedInState() });

    expect(await screen.findByText('Bilderwiedergabe')).toBeInTheDocument();
  });
});

describe('sign-in', () => {
  it('signs in with an OTP and lands on the home page', async () => {
    const spy = spyOnRequests('post', api.getLoginUrl());
    server.use(spy.handler);
    const { user, router } = await openSignIn();

    await user.type(screen.getByLabelText(/OTP/), '123456');
    await user.click(await screen.findByRole('button', { name: 'Anmelden' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/'));
    expect(await spy.calls[0].json()).toEqual({ otp: '123456' });
    expect(await screen.findByText('Authentifizierung erfolgreich')).toBeInTheDocument();
  });

  it('signs in with the device password', async () => {
    const spy = spyOnRequests('post', api.getLoginUrl());
    server.use(spy.handler);
    const { user, router } = await openSignIn();

    await user.click(screen.getByRole('button', { name: 'Mit Passwort anmelden' }));
    await user.type(screen.getByLabelText(/Passwort/), 'geheim123');
    await user.click(screen.getByRole('button', { name: 'Anmelden' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/'));
    expect(await spy.calls[0].json()).toEqual({ password: 'geheim123' });
  });

  it('shows the reason of a rejected login and stays on the page', async () => {
    server.use(...failsWith('post', api.getLoginUrl(), 401, 'OTP invalid'));
    const { user, router } = await openSignIn();

    await user.type(screen.getByLabelText(/OTP/), '000000');
    await user.click(await screen.findByRole('button', { name: 'Anmelden' }));

    expect(await screen.findByText('OTP invalid')).toBeInTheDocument();
    expect(router.state.location.pathname).toBe(SIGN_IN);
  });

  it('cannot submit while the board is unreachable', async () => {
    server.use(...boardUnreachable());
    const { user } = await openSignIn();

    await user.type(screen.getByLabelText(/OTP/), '123456');

    expect(screen.getByRole('button', { name: 'Anmelden' })).toBeDisabled();
  });

  it('switches to the password login when the board has no internet', async () => {
    server.use(...withConnectionMode({ internet: false }));
    await openSignIn();

    expect(await screen.findByText(/die OTP-Anmeldung ist derzeit nicht möglich/)).toBeInTheDocument();
    expect(screen.getByLabelText(/Passwort/)).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Mit OTP anmelden' })).toBeInTheDocument();
  });
});

describe('sign-out', () => {
  it('signs out and returns to the sign-in page', async () => {
    const spy = spyOnRequests('post', api.getLogoutUrl());
    server.use(spy.handler);
    const { user, router, store } = renderRoute('/auth/sign-out', { preloadedState: signedInState() });

    await user.click(await screen.findByRole('button', { name: 'Abmelden' }));

    await waitFor(() => expect(router.state.location.pathname).toBe(SIGN_IN));
    expect(spy.calls).toHaveLength(1);
    expect(store.getState().auth.isAuthenticated).toBe(false);
    expect(await screen.findByText('Erfolgreich ausgeloggt')).toBeInTheDocument();
  });

  it('drops stale alerts when the session ran out', async () => {
    const store = setupStore(buildState());
    store.dispatch(addAlertSnackbar('stale', 'Abrufen der Logs fehlgeschlagen', 'error'));
    renderRoute('/logs/', { store });

    await screen.findByText('Willkommen bei deinem Bilderrahmen');

    expect(screen.queryByText('Abrufen der Logs fehlgeschlagen')).not.toBeInTheDocument();
  });
});
