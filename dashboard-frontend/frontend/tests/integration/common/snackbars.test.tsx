import { afterEach, describe, expect, it, vi } from 'vitest';
import { screen, within } from '@testing-library/react';
import { Route, Routes } from 'react-router';
import Snackbars from '@/common/components/snackbars/snackbars';
import { addAlertSnackbar, addLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { setupStore } from '@/store/setupStore';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';
import { advance, useBoardClock } from '@tests/helpers/time';

const renderSnackbars = (store = setupStore()) =>
  renderWithProviders(
    <Routes>
      <Route element={<Snackbars />}>
        <Route path="/" element={<p>Seite</p>} />
      </Route>
    </Routes>,
    { store },
  );

describe('snackbars', () => {
  afterEach(() => vi.useRealTimers());

  it('renders the page and every alert', async () => {
    const store = setupStore();
    renderSnackbars(store);

    store.dispatch(addAlertSnackbar('a', 'Gespeichert', 'success'));
    store.dispatch(addAlertSnackbar('b', 'Fehlgeschlagen', 'error'));

    expect(screen.getByText('Seite')).toBeInTheDocument();
    expect(await screen.findByText('Gespeichert')).toBeInTheDocument();
    expect(screen.getByText('Fehlgeschlagen')).toBeInTheDocument();
  });

  it('closes an alert on request', async () => {
    const store = setupStore();
    store.dispatch(addAlertSnackbar('a', 'Gespeichert', 'success'));
    const { user } = renderSnackbars(store);

    const alert = (await screen.findByText('Gespeichert')).parentElement as HTMLElement;
    await user.click(within(alert).getByRole('button', { name: 'close' }));

    expect(screen.queryByText('Gespeichert')).not.toBeInTheDocument();
    expect(store.getState().snackbars.snackbars.alerts).toEqual([]);
  });

  it('hides an alert after its duration', async () => {
    useBoardClock();
    const store = setupStore();
    renderSnackbars(store);
    store.dispatch(addAlertSnackbar('a', 'Gleich weg', 'info', 2_000));
    await screen.findByText('Gleich weg');

    await advance(2_000);

    expect(screen.queryByText('Gleich weg')).not.toBeInTheDocument();
  });

  // A loading snackbar left over from before a reload would spin forever.
  it('drops stale loading snackbars on mount', () => {
    const store = setupStore();
    store.dispatch(addLoadingSnackbar('l', 'Lädt noch'));

    renderSnackbars(store);

    expect(screen.queryByText('Lädt noch')).not.toBeInTheDocument();
    expect(store.getState().snackbars.snackbars.loading).toEqual([]);
  });
});
