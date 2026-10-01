import { describe, expect, it } from 'vitest';
import reducer, {
  addAlertSnackbar,
  addLoadingSnackbar,
  removeAlertSnackbar,
  removeAllLoadingSnackbars,
  removeAllSnackbars,
  removeLoadingSnackbar,
} from '@/store/snackbars/snackbars.Slice';

const initial = () => reducer(undefined, { type: '@@tests/INIT' });

const withAlerts = (count: number) =>
  Array.from({ length: count }, (_, i) => addAlertSnackbar(`a${i}`, `alert ${i}`, 'info')).reduce(reducer, initial());

describe('snackbars slice', () => {
  it('adds and removes alerts by id', () => {
    let state = reducer(initial(), addAlertSnackbar('a', 'Hallo', 'success', 3000));
    expect(state.snackbars.alerts).toEqual([{ id: 'a', message: 'Hallo', severity: 'success', autoHideDuration: 3000 }]);

    state = reducer(state, removeAlertSnackbar('a'));
    expect(state.snackbars.alerts).toEqual([]);
  });

  it('keeps at most five alerts, dropping the oldest', () => {
    const state = reducer(withAlerts(5), addAlertSnackbar('new', 'neu', 'error'));

    expect(state.snackbars.alerts).toHaveLength(5);
    expect(state.snackbars.alerts[0].id).toBe('a1');
    expect(state.snackbars.alerts[4].id).toBe('new');
  });

  it('adds and removes loading snackbars by id', () => {
    let state = reducer(initial(), addLoadingSnackbar('l', 'Lädt'));
    expect(state.snackbars.loading).toEqual([{ id: 'l', message: 'Lädt', autoHideDuration: undefined }]);

    state = reducer(state, removeLoadingSnackbar('l'));
    expect(state.snackbars.loading).toEqual([]);
  });

  it('clears only the loading snackbars', () => {
    let state = reducer(withAlerts(1), addLoadingSnackbar('l', 'Lädt'));
    state = reducer(state, removeAllLoadingSnackbars());

    expect(state.snackbars.loading).toEqual([]);
    expect(state.snackbars.alerts).toHaveLength(1);
  });

  it('clears everything', () => {
    let state = reducer(withAlerts(2), addLoadingSnackbar('l', 'Lädt'));
    state = reducer(state, removeAllSnackbars());

    expect(state.snackbars).toEqual({ alerts: [], loading: [] });
  });
});
