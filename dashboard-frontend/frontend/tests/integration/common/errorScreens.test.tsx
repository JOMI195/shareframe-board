import { describe, expect, it, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import { createMemoryRouter, RouterProvider } from 'react-router';
import RouteError from '@/common/components/error/routeError/routeError';
import { setupStore } from '@/store/setupStore';
import { AppProviders } from '@tests/helpers/providers';

const Broken = () => {
  throw new Error('render failed');
};

describe('error screens', () => {
  it('shows a German error page with a way back when a page crashes', async () => {
    vi.spyOn(console, 'error').mockImplementation(() => {});
    const router = createMemoryRouter([{ path: '/', element: <Broken />, errorElement: <RouteError /> }]);

    render(
      <AppProviders store={setupStore()}>
        <RouterProvider router={router} />
      </AppProviders>,
    );

    expect(await screen.findByRole('heading', { level: 1, name: 'Etwas ist schiefgelaufen' })).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Neu laden' })).toBeInTheDocument();
    expect(screen.getByRole('link', { name: 'Zur Übersicht' })).toHaveAttribute('href', '/');
  });
});
