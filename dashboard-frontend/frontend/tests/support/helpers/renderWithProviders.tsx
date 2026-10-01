import { ReactElement, ReactNode } from 'react';
import { MemoryRouter } from 'react-router';
import { render, RenderOptions } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { setupStore, TestStore, RootReducerState } from '@/store/setupStore';
import { buildState, DeepPartial } from './preloadedState';
import { AppProviders } from './providers';

export interface ProviderOptions {
  preloadedState?: RootReducerState | DeepPartial<RootReducerState>;
  store?: TestStore;
  route?: string;
}

// No PersistGate: persistence is covered by the migration tests, and it makes every render async.
export const createWrapper = (options: ProviderOptions = {}) => {
  const {
    preloadedState,
    store = setupStore(preloadedState ? buildState(preloadedState) : undefined),
    route = '/',
  } = options;

  const Wrapper = ({ children }: { children: ReactNode }) => (
    <AppProviders store={store}>
      <MemoryRouter initialEntries={[route]}>{children}</MemoryRouter>
    </AppProviders>
  );

  return { store, Wrapper };
};

export const renderWithProviders = (
  ui: ReactElement,
  options: ProviderOptions & Omit<RenderOptions, 'wrapper'> = {},
) => {
  const { preloadedState, store: given, route, ...renderOptions } = options;
  const { store, Wrapper } = createWrapper({ preloadedState, store: given, route });

  return {
    store,
    user: userEvent.setup(),
    ...render(ui, { wrapper: Wrapper, ...renderOptions }),
  };
};
