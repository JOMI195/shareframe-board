import { RouterProvider, createMemoryRouter } from 'react-router';
import { render } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { routes } from '@/routes/routing';
import { setupStore, type RootReducerState, type TestStore } from '@/store/setupStore';
import { buildState, type DeepPartial } from './preloadedState';
import { AppProviders } from './providers';

export interface RenderRouteOptions {
  preloadedState?: RootReducerState | DeepPartial<RootReducerState>;
  store?: TestStore;
}

/**
 * Mounts the real route table at `path`. Use this for anything that spans more
 * than one screen; single components go through renderWithProviders.
 */
export const renderRoute = (path: string, options: RenderRouteOptions = {}) => {
  const { preloadedState, store = setupStore(preloadedState ? buildState(preloadedState) : undefined) } =
    options;
  const router = createMemoryRouter(routes, { initialEntries: [path] });

  return {
    store,
    router,
    user: userEvent.setup(),
    ...render(
      <AppProviders store={store}>
        <RouterProvider router={router} />
      </AppProviders>,
    ),
  };
};
