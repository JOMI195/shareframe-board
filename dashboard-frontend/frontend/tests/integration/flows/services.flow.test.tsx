import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { card, findCard } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { healthyServices, makeService } from '@tests/fixtures';
import { failsWith, server, spyOnRequests, withLogs, withServices } from '@tests/mocks';

describe('services overview', () => {
  it('shows every managed service with its state', async () => {
    server.use(
      ...withServices([
        ...healthyServices().filter((s) => s.id !== 'websocket'),
        makeService('websocket', { running: false, status: 'down', uptime_seconds: 42 }),
      ]),
    );
    renderRoute('/services/', { preloadedState: signedInState() });

    await within(await findCard('WebSocket')).findByText('Offline');
    expect(within(card('WebSocket')).getByText('down · 42s')).toBeInTheDocument();
    expect(within(card('Display')).getByText('Aktiv')).toBeInTheDocument();
    expect(within(card('Display')).getByText('up · 2h 0m')).toBeInTheDocument();
  });

  it('marks services the board did not report as unknown', async () => {
    server.use(...withServices([]));
    renderRoute('/services/', { preloadedState: signedInState() });

    expect(await screen.findAllByText('Unbekannt')).toHaveLength(5);
  });

  it('restarts a service after confirmation', async () => {
    const restart = spyOnRequests('post', api.getServiceRestartUrl());
    server.use(restart.handler);
    const { user } = renderRoute('/services/', { preloadedState: signedInState() });

    await user.click(within(await findCard('Heartbeat')).getByRole('button', { name: 'Neustart' }));
    const dialog = await screen.findByRole('dialog');
    expect(within(dialog).getByText(/Heartbeat wird neu gestartet/)).toBeInTheDocument();
    await user.click(within(dialog).getByRole('button', { name: 'Neustart' }));

    expect(await screen.findByText('Dienst wird neu gestartet')).toBeInTheDocument();
    expect(await restart.calls[0].json()).toEqual({ service: 'heartbeat' });
  });

  it('warns that restarting the dashboard drops the connection', async () => {
    const { user } = renderRoute('/services/', { preloadedState: signedInState() });

    await user.click(within(await findCard('Dashboard')).getByRole('button', { name: 'Neustart' }));

    expect(within(await screen.findByRole('dialog')).getByText(/Das Dashboard ist dabei kurz nicht erreichbar/)).toBeInTheDocument();
  });

  it('shows the reason of a refused restart', async () => {
    server.use(...failsWith('post', api.getServiceRestartUrl(), 400, 'Unbekannter Dienst'));
    const { user } = renderRoute('/services/', { preloadedState: signedInState() });

    await user.click(within(await findCard('Display')).getByRole('button', { name: 'Neustart' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Neustart' }));

    expect(await screen.findByText('Unbekannter Dienst')).toBeInTheDocument();
  });

  it('opens the detail page of a service', async () => {
    const { user, router } = renderRoute('/services/', { preloadedState: signedInState() });

    await user.click(within(await findCard('Display')).getByRole('link', { name: 'Details' }));

    await waitFor(() => expect(router.state.location.pathname).toBe('/services/display'));
  });
});

describe('service detail', () => {
  it('shows the service state and its logs', async () => {
    const logs = spyOnRequests('get', api.getFrameLogsUrl(), () =>
      Response.json({ success: true, message: '', data: { service_name: 'display', lines: 2, logs: 'refresh ok\nrefresh ok again\n' } }),
    );
    server.use(logs.handler);
    renderRoute('/services/display', { preloadedState: signedInState() });

    const overview = card('Display');
    expect(await within(overview).findByText('Aktiv')).toBeInTheDocument();
    expect(within(overview).getByText('100')).toBeInTheDocument();
    expect(await screen.findByText('refresh ok again')).toBeInTheDocument();
    expect(screen.getByText('2', { selector: 'strong' })).toBeInTheDocument();

    const query = new URL(logs.calls[0].url).searchParams;
    expect(query.get('service_name')).toBe('display');
    expect(query.get('lines')).toBe('500');
    expect(query.get('since_timestamp')).toMatch(/^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d$/);
  });

  it('says so when the service logged nothing', async () => {
    server.use(...withLogs([]));
    renderRoute('/services/websocket', { preloadedState: signedInState() });

    expect(await screen.findByText('Keine Einträge gefunden')).toBeInTheDocument();
  });
});
