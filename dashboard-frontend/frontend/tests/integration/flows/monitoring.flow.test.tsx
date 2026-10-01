import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { card, findCard } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeDisplayStats } from '@tests/fixtures';
import { failsWith, ok, server, spyOnRequests, withDisplayStats } from '@tests/mocks';

describe('system logs', () => {
  const systemLogs = (logs = 'boot ok\nwlan0 up\n') =>
    spyOnRequests('get', api.getFrameLogsUrl(), () => ok({ service_name: 'system', lines: 2, logs }));

  it('loads the system log of the last three hours', async () => {
    const logs = systemLogs();
    server.use(logs.handler);
    renderRoute('/logs/', { preloadedState: signedInState() });

    expect(await screen.findByText('wlan0 up')).toBeInTheDocument();
    const query = new URL(logs.calls[0].url).searchParams;
    expect(query.get('service_name')).toBe('system');
    expect(query.get('lines')).toBe('1000');
  });

  it('refetches with a new line limit', async () => {
    const logs = systemLogs();
    server.use(logs.handler);
    const { user } = renderRoute('/logs/', { preloadedState: signedInState() });
    await screen.findByText('wlan0 up');

    const lines = screen.getByLabelText('Anzahl Zeilen');
    await user.clear(lines);
    await user.type(lines, '50');

    await waitFor(() => expect(new URL(logs.calls[logs.calls.length - 1].url).searchParams.get('lines')).toBe('50'));
  });

  it('restores the last valid line limit when the field is left empty', async () => {
    const logs = systemLogs();
    server.use(logs.handler);
    const { user } = renderRoute('/logs/', { preloadedState: signedInState() });
    await screen.findByText('wlan0 up');
    const requests = logs.calls.length;

    const lines = screen.getByLabelText('Anzahl Zeilen');
    await user.clear(lines);
    expect(lines).toHaveValue(null);
    await user.tab();

    expect(lines).toHaveValue(1000);
    expect(logs.calls).toHaveLength(requests);
  });

  it('asks for a start date when the custom range is picked', async () => {
    const { user } = renderRoute('/logs/', { preloadedState: signedInState() });

    await user.click(await screen.findByRole('combobox', { name: 'Zeitraum' }));
    await user.click(await screen.findByRole('option', { name: 'Benutzerdefiniert' }));

    expect(screen.getByLabelText('Datum und Uhrzeit')).toBeInTheDocument();
  });

  it('reports a failed load', async () => {
    server.use(...failsWith('get', api.getFrameLogsUrl()));
    renderRoute('/logs/', { preloadedState: signedInState() });

    expect(await screen.findByText('Abrufen der Logs fehlgeschlagen')).toBeInTheDocument();
    expect(screen.getByText('Keine Protokolle verfügbar')).toBeInTheDocument();
  });
});

describe('display health', () => {
  it('shows the panel state and its counters', async () => {
    server.use(...withDisplayStats(makeDisplayStats({ epd_refresh_total: 12_345, wear_percent: 1.2345, epd_busy_ms_total: 3_723_000 })));
    renderRoute('/display-health/', { preloadedState: signedInState() });

    expect(await within(await findCard('Zustand')).findByText('OK')).toBeInTheDocument();
    expect(within(card('Verschleiß')).getByText('12.345 (1.2345 %)')).toBeInTheDocument();
    expect(within(card('Nutzung')).getByText('1h 2m 3s')).toBeInTheDocument();
  });

  it.each([
    ['degraded', 'Eingeschränkt'],
    ['failed', 'Ausgefallen'],
  ] as const)('labels a %s panel', async (health, label) => {
    server.use(...withDisplayStats(makeDisplayStats({ health })));
    renderRoute('/display-health/', { preloadedState: signedInState() });

    expect(await within(await findCard('Zustand')).findByText(label)).toBeInTheDocument();
  });

  it('shows dashes for counters the panel has not recorded yet', async () => {
    server.use(...withDisplayStats({ health: 'ok', wear_percent: 0 }));
    renderRoute('/display-health/', { preloadedState: signedInState() });

    const reliability = await findCard('Zuverlässigkeit');
    await waitFor(() => expect(within(reliability).getAllByText('—')).toHaveLength(3));
  });
});
