import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeRelease } from '@tests/fixtures';
import { boardUnreachable, server, withConnectionMode, withRelease } from '@tests/mocks';

const sidebar = () => screen.getByRole('sidebar');

describe('navigation', () => {
  it('opens a page from the sidebar', async () => {
    const { user, router } = renderRoute('/', { preloadedState: signedInState({ navigation: { sidebar: { open: true } } }) });

    await user.click(within(sidebar()).getByRole('link', { name: 'Netzwerk' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/network/'));
    expect(await screen.findByRole('heading', { name: 'Übersicht (WIFI)' })).toBeInTheDocument();
  });

  it('reveals the admin pages in advanced mode', async () => {
    const { user } = renderRoute('/', { preloadedState: signedInState({ navigation: { sidebar: { open: true } } }) });

    expect(within(sidebar()).queryByRole('link', { name: 'Protokolle' })).not.toBeInTheDocument();

    await user.click(within(sidebar()).getByText('Erweitert'));

    expect(within(sidebar()).getByRole('link', { name: 'Protokolle' })).toBeInTheDocument();
    expect(within(sidebar()).getByText('Verwaltung')).toBeInTheDocument();
  });

  it('toggles the sidebar from the top bar', async () => {
    const { user, store } = renderRoute('/', { preloadedState: signedInState() });

    await user.click(await screen.findByRole('button', { name: 'toggle sidebar' }));
    expect(store.getState().navigation.sidebar.open).toBe(true);

    await user.click(screen.getByRole('button', { name: 'toggle sidebar' }));
    expect(store.getState().navigation.sidebar.open).toBe(false);
  });

  it('shows a 404 for unknown pages', async () => {
    renderRoute('/nope', { preloadedState: signedInState() });

    expect(await screen.findByText('Page not found')).toBeInTheDocument();
  });

  it('switches the color mode and remembers it', async () => {
    const { user } = renderRoute('/', { preloadedState: signedInState() });

    await user.click(await screen.findByRole('button', { name: 'Wechsel in den dunklen Modus' }));

    expect(localStorage.getItem('color-mode')).toBe('dark');
    expect(screen.getByRole('button', { name: 'Wechsel in den hellen Modus' })).toBeInTheDocument();
  });
});

describe('notifications', () => {
  const openNotifications = async (badge = '') => {
    const view = renderRoute('/', { preloadedState: signedInState() });
    const button = await screen.findByRole('button', { name: 'Benachrichtigungen' });
    if (badge) await waitFor(() => expect(button).toHaveTextContent(badge));
    await view.user.click(button);
    return within(await screen.findByRole('menu'));
  };

  it('has nothing to report on a healthy, current board', async () => {
    const menu = await openNotifications();

    expect(menu.getByText('Keine Benachrichtigungen verfügbar')).toBeInTheDocument();
  });

  it('announces a newer version', async () => {
    server.use(...withRelease(makeRelease({ version: '9.0.0' })));

    const menu = await openNotifications('1');

    expect(menu.getByText('Neue Version (9.0.0) verfügbar')).toBeInTheDocument();
  });

  it('warns when the board does not answer', async () => {
    server.use(...boardUnreachable());

    const menu = await openNotifications('1');

    expect(menu.getByText('Keine Verbindung zum Bilderrahmen')).toBeInTheDocument();
  });
});

describe('network status banner', () => {
  it('stays hidden while the board is online', async () => {
    renderRoute('/', { preloadedState: signedInState() });
    await screen.findByText('Bilderwiedergabe');

    expect(screen.queryByRole('status')).not.toBeInTheDocument();
  });

  it.each([
    [{ mode: 'connecting' as const, ssid: 'HomeNet' }, 'Verbinde…', 'Verbindung zu „HomeNet“ wird hergestellt.'],
    [{ mode: 'connecting' as const, ssid: '' }, 'Verbinde…', 'Suche nach bekannten Netzwerken.'],
    [{ mode: 'connected' as const, internet: false }, 'Kein Internet', 'Mit „HomeNet“ verbunden, aber keine Internetverbindung.'],
    [{ mode: 'ap' as const, ap_ssid: 'sf-1', ap_password: 'pw123456' }, 'AP-Modus aktiv', /WLAN „sf-1“ · Passwort: pw123456/],
  ])('explains %o', async (mode, title, detail) => {
    server.use(...withConnectionMode(mode));
    renderRoute('/', { preloadedState: signedInState() });

    const banner = await screen.findByRole('status');
    expect(within(banner).getByText(title)).toBeInTheDocument();
    expect(within(banner).getByText(detail)).toBeInTheDocument();
  });
});
