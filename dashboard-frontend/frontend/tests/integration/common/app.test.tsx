import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import App from '@/App';
import { showLoadingWall } from '@/store/loadingWall/loadingWall.Slice';
import { setupStore } from '@/store/setupStore';
import { AppProviders } from '@tests/helpers/providers';
import { advance, useBoardClock } from '@tests/helpers/time';
import { boardUnreachable, server, signedOut } from '@tests/mocks';

// The real browser router reads window.location, which the setup replaces.
vi.mock('@/routes/routing', async () => {
  const { createElement } = await import('react');
  const { createMemoryRouter } = await import('react-router');
  return { default: createMemoryRouter([{ path: '*', element: createElement('p', null, 'Router') }]) };
});

const renderApp = (store = setupStore()) => {
  render(
    <AppProviders store={store}>
      <App />
    </AppProviders>,
  );
  return store;
};

const authChecked = (store: ReturnType<typeof setupStore>) =>
  waitFor(() => expect(store.getState().auth.isAuthenticated).toBe(true));

describe('App', () => {
  beforeEach(() => {
    useBoardClock();
    vi.spyOn(console, 'error').mockImplementation(() => undefined);
  });

  afterEach(() => vi.useRealTimers());

  it('shows the splash for three seconds after the session check', async () => {
    const store = renderApp();

    expect(screen.getByRole('progressbar')).toBeInTheDocument();
    await authChecked(store);
    await advance(2_900);
    expect(screen.queryByText('Router')).not.toBeInTheDocument();

    await advance(100);

    expect(screen.getByText('Router')).toBeInTheDocument();
  });

  it('rechecks the session when the tab regains focus', async () => {
    const store = renderApp();
    await authChecked(store);
    await advance(3_000);

    server.use(...signedOut());
    window.dispatchEvent(new Event('focus'));

    await waitFor(() => expect(store.getState().auth.isAuthenticated).toBe(false));
  });

  it('keeps the loading wall up until the board comes back', async () => {
    server.use(...boardUnreachable());
    const store = setupStore();
    store.dispatch(showLoadingWall('Neustart läuft'));
    renderApp(store);

    await advance(3_000);
    expect(screen.getByText('Neustart läuft')).toBeInTheDocument();

    server.resetHandlers();
    await advance(30_000);

    expect(screen.queryByText('Neustart läuft')).not.toBeInTheDocument();
    expect(screen.getByText('Router')).toBeInTheDocument();
  });

  it('drops the loading wall at its deadline even if the board stays away', async () => {
    server.use(...boardUnreachable());
    const store = setupStore();
    store.dispatch(showLoadingWall('Neustart läuft'));
    renderApp(store);

    await advance(3 * 60_000);

    expect(screen.queryByText('Neustart läuft')).not.toBeInTheDocument();
  });
});
