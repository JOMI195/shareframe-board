import { afterEach, describe, expect, it, vi } from 'vitest';
import { screen, waitFor } from '@testing-library/react';
import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';
import { advance, useBoardClock } from '@tests/helpers/time';
import { apiUrl, boardUnreachable, ok, server } from '@tests/mocks';

const Probe = () => {
  const { isConnected } = usePiConnection();
  return <p>{isConnected ? 'verbunden' : 'getrennt'}</p>;
};

describe('board connection', () => {
  afterEach(() => vi.useRealTimers());

  it('is connected once the health check answers', async () => {
    renderWithProviders(<Probe />);

    expect(await screen.findByText('verbunden')).toBeInTheDocument();
  });

  it('is disconnected while the board does not answer', async () => {
    vi.spyOn(console, 'error').mockImplementation(() => undefined);
    server.use(...boardUnreachable());
    renderWithProviders(<Probe />);

    await waitFor(() => expect(console.error).toHaveBeenCalled());
    expect(screen.getByText('getrennt')).toBeInTheDocument();
  });

  it('treats a board that is not running as disconnected', async () => {
    server.use(http.get(apiUrl(api.getSystemHealthUrl()), () => ok({ running: false })));
    renderWithProviders(<Probe />);

    await waitFor(() => expect(screen.getByText('getrennt')).toBeInTheDocument());
  });

  it('rechecks every 30 seconds', async () => {
    useBoardClock();
    vi.spyOn(console, 'error').mockImplementation(() => undefined);
    server.use(...boardUnreachable());
    renderWithProviders(<Probe />);
    await advance(0);
    expect(screen.getByText('getrennt')).toBeInTheDocument();

    server.resetHandlers();
    await advance(30_000);

    expect(await screen.findByText('verbunden')).toBeInTheDocument();
  });
});
