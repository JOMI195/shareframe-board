import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeRelease } from '@tests/fixtures';
import * as api from '@/assets/endpoints/api/frame';
import { boardUnreachable, server, spyOnRequests, withConnectionMode, withRelease } from '@tests/mocks';

const sidebar = () => screen.getByRole('navigation', { name: 'Hauptmenü' });

const openMenu = async (user: { click: (element: Element) => Promise<void> }) => {
  await user.click(await screen.findByRole('button', { name: 'Menü öffnen' }));
};

describe('navigation', () => {
  it('opens a page from the sidebar', async () => {
    const { user, router } = renderRoute('/', { preloadedState: signedInState() });
    await openMenu(user);

    await user.click(within(sidebar()).getByRole('link', { name: 'Netzwerk' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/network/'));
    expect(await screen.findByRole('heading', { name: 'WLAN' })).toBeInTheDocument();
  });

  it('reveals the admin pages once expert mode is on in the system page', async () => {
    const { user } = renderRoute('/', { preloadedState: signedInState() });
    await openMenu(user);

    expect(within(sidebar()).queryByRole('link', { name: 'Protokolle' })).not.toBeInTheDocument();

    await user.click(within(sidebar()).getByRole('link', { name: 'System' }));
    await user.click(await screen.findByRole('switch', { name: 'Expertenmodus' }));
    await openMenu(user);

    expect(within(sidebar()).getByRole('link', { name: 'Protokolle' })).toBeInTheDocument();
    expect(within(sidebar()).getByText('Verwaltung')).toBeInTheDocument();
  });

  it('toggles the sidebar from the top bar', async () => {
    const { user, store } = renderRoute('/', { preloadedState: signedInState() });

    const toggle = await screen.findByRole('button', { name: 'Menü öffnen' });
    expect(toggle).toHaveAttribute('aria-expanded', 'false');

    await user.click(toggle);
    expect(store.getState().navigation.sidebar.open).toBe(true);
    expect(toggle).toHaveAccessibleName('Menü schließen');
    expect(toggle).toHaveAttribute('aria-expanded', 'true');

    await user.keyboard('{Escape}');
    expect(store.getState().navigation.sidebar.open).toBe(false);
  });

  it('closes the menu after picking a page and names the page in the title', async () => {
    const { user, store } = renderRoute('/', { preloadedState: signedInState() });
    await openMenu(user);

    await user.click(within(sidebar()).getByRole('link', { name: 'Diashow' }));

    await waitFor(() => expect(store.getState().navigation.sidebar.open).toBe(false));
    expect(await screen.findByRole('heading', { level: 1, name: 'Diashow' })).toBeInTheDocument();
    expect(document.title).toBe('Diashow – ShareFrame');
    // The closed phone menu stays mounted but hidden, so it is out of reach for role queries.
    expect(document.querySelector('nav[aria-label="Hauptmenü"] [aria-current="page"]')).toHaveTextContent('Diashow');
  });

  it('shows a 404 for unknown pages', async () => {
    renderRoute('/nope', { preloadedState: signedInState() });

    expect(await screen.findByText('Seite nicht gefunden')).toBeInTheDocument();
  });

  it('switches the color mode and remembers it', async () => {
    const { user } = renderRoute('/', { preloadedState: signedInState() });

    await user.click(await screen.findByRole('button', { name: 'Wechsel in den dunklen Modus' }));

    expect(localStorage.getItem('color-mode')).toBe('dark');
    expect(screen.getByRole('button', { name: 'Wechsel in den hellen Modus' })).toBeInTheDocument();
  });
});

describe('system status', () => {
  const openSystem = async (label: string | RegExp = /^System/) => {
    const view = renderRoute('/', { preloadedState: signedInState() });
    await view.user.click(await screen.findByRole('button', { name: label }));
    return { ...view, popover: within(await screen.findByRole('dialog', { name: 'System' })) };
  };

  it('shows network, server and board on a healthy, current board', async () => {
    const { popover } = await openSystem('System: Online');

    expect(popover.getByText('Mit „HomeNet“ verbunden')).toBeInTheDocument();
    expect(popover.getByText('Mit dem Server verbunden')).toBeInTheDocument();
    expect(popover.getByText('Bilderrahmen erreichbar')).toBeInTheDocument();
    expect(popover.queryByText(/Neue Version/)).not.toBeInTheDocument();
  });

  it.each([
    [{}, 'Online'],
    [{ mode: 'connected' as const, internet: false }, 'Kein Internet'],
    [{ mode: 'connecting' as const }, 'Verbinde …'],
    [{ mode: 'ap' as const, internet: false }, 'Hotspot'],
  ])('names %o as %s in the header', async (mode, label) => {
    server.use(...withConnectionMode(mode));
    renderRoute('/', { preloadedState: signedInState() });

    expect(await screen.findByRole('button', { name: new RegExp(`^System: ${label}`) })).toBeInTheDocument();
  });

  it('spells out the hotspot without a hover', async () => {
    server.use(...withConnectionMode({ mode: 'ap', internet: false, ap_ssid: 'sf-1' }));

    const { popover } = await openSystem('System: Hotspot');

    expect(popover.getByText('Rahmen stellt den Hotspot „sf-1“ bereit')).toBeInTheDocument();
  });

  it('says how long the server has been unreachable', async () => {
    server.use(...withConnectionMode({ server_connected: false, server_offline_secs: 3 * 3600, server_last_contact_at: 1_760_000_000 }));

    const { popover } = await openSystem('System: Online (1 Hinweis)');

    expect(popover.getByText('Server nicht erreichbar')).toBeInTheDocument();
    expect(popover.getByText('Letzter Kontakt vor 3 Stunden')).toBeInTheDocument();
  });

  it('announces a newer version and opens the updates page', async () => {
    server.use(...withRelease(makeRelease({ version: '9.0.0' })));

    const { popover, user, router } = await openSystem('System: Online (1 Hinweis)');
    await user.click(popover.getByRole('button', { name: /Neue Version \(9\.0\.0\) verfügbar/ }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/updates/'));
  });

  it('warns when the board does not answer', async () => {
    server.use(...boardUnreachable());

    const { popover } = await openSystem('System: Online (1 Hinweis)');

    expect(popover.getByText('Keine Verbindung zum Bilderrahmen')).toBeInTheDocument();
  });
});

describe('network status banner', () => {
  it('stays hidden while the board is online', async () => {
    renderRoute('/', { preloadedState: signedInState() });
    await screen.findByRole('heading', { level: 2, name: 'Diashow' });

    expect(screen.queryByRole('status')).not.toBeInTheDocument();
  });

  it.each([
    [{ mode: 'connecting' as const, ssid: 'HomeNet' }, 'Verbindet …', 'Verbindung zu „HomeNet“ wird hergestellt.'],
    [{ mode: 'connecting' as const, ssid: '' }, 'Verbindet …', 'Suche nach bekannten Netzwerken.'],
    [{ mode: 'connected' as const, internet: false }, 'Kein Internet', 'Mit „HomeNet“ verbunden, aber keine Internetverbindung.'],
    [{ mode: 'ap' as const, ap_ssid: 'sf-1', ap_password: 'pw123456' }, 'Hotspot aktiv', /^Der Bilderrahmen stellt den Hotspot „sf-1“ bereit\. Füge unter „Netzwerk“/],
  ])('explains %o', async (mode, title, detail) => {
    server.use(...withConnectionMode(mode));
    renderRoute('/', { preloadedState: signedInState() });

    const banner = await screen.findByRole('status');
    expect(within(banner).getByText(title)).toBeInTheDocument();
    expect(within(banner).getByText(detail)).toBeInTheDocument();
  });

  it('says how long the server has been unreachable', async () => {
    server.use(...withConnectionMode({ server_connected: false, server_offline_secs: 3 * 3600, server_last_contact_at: 1_760_000_000 }));
    renderRoute('/', { preloadedState: signedInState() });

    const banner = await screen.findByRole('status');
    expect(within(banner).getByText('Server nicht erreichbar')).toBeInTheDocument();
    expect(within(banner).getByText(/^Letzter Kontakt vor 3 Stunden\./)).toBeInTheDocument();
  });

  it('tells the owner that nobody has added the frame yet', async () => {
    server.use(...withConnectionMode({ registered: false, server_connected: false, server_offline_secs: 0, server_last_contact_at: 0 }));
    renderRoute('/', { preloadedState: signedInState() });

    const banner = await screen.findByRole('status');
    expect(within(banner).getByText('Noch nicht registriert')).toBeInTheDocument();
    expect(within(banner).getByRole('link', { name: 'shareframe.de' })).toHaveAttribute('href', 'https://www.shareframe.de/');
  });

  it('flags a frame that has never reached the server', async () => {
    server.use(...withConnectionMode({ server_connected: false, server_offline_secs: 0, server_last_contact_at: 0 }));
    renderRoute('/', { preloadedState: signedInState() });

    expect(within(await screen.findByRole('status')).getByText(/^Noch keine Verbindung zum Server/)).toBeInTheDocument();
  });

  it.each([
    [{ server_connected: false, server_offline_secs: 30 }],
    [{ server_connected: null }],
  ])('keeps quiet for a short reconnect or an unknown link: %o', async (over) => {
    server.use(...withConnectionMode(over));
    renderRoute('/', { preloadedState: signedInState() });
    await screen.findByRole('heading', { level: 2, name: 'Diashow' });

    expect(screen.queryByRole('status')).not.toBeInTheDocument();
  });
});

describe('hotspot notice', () => {
  it('keeps the hotspot password off the notice and leaves the network page to its own card', async () => {
    server.use(...withConnectionMode({ mode: 'ap', internet: false, ap_ssid: 'sf-1', ap_password: 'pw123456' }));
    const { router } = renderRoute('/', { preloadedState: signedInState() });

    const notice = await screen.findByRole('status');
    expect(notice).not.toHaveTextContent('pw123456');

    await router.navigate('/network/');
    await screen.findByRole('heading', { level: 2, name: 'WLAN einrichten' });
    expect(screen.queryByRole('status')).not.toBeInTheDocument();
  });
});

describe('safe mode notice', () => {
  it('says why the panel stays blank and turns the display back on', async () => {
    const spy = spyOnRequests('post', api.getExitSafeModeUrl());
    server.use(...withConnectionMode({ safe_mode: 'display-crash' }), spy.handler);
    const { user } = renderRoute('/', { preloadedState: signedInState() });

    const notice = await screen.findByRole('region', { name: 'Anzeige ausgeschaltet' });
    expect(within(notice).getByText(/mehrmals abgestürzt/)).toBeInTheDocument();
    await user.click(within(notice).getByRole('button', { name: 'Anzeige wieder einschalten' }));

    await waitFor(() => expect(spy.calls).toHaveLength(1));
    expect(await screen.findByText('Die Anzeige wird wieder eingeschaltet.')).toBeInTheDocument();
  });

  it('stays away while the display runs', async () => {
    renderRoute('/', { preloadedState: signedInState() });
    await screen.findByRole('heading', { level: 2, name: 'Diashow' });

    expect(screen.queryByRole('region', { name: 'Anzeige ausgeschaltet' })).not.toBeInTheDocument();
  });
});
