import '@testing-library/jest-dom/vitest';
import { cleanup } from '@testing-library/react';
import { afterAll, afterEach, beforeAll, vi } from 'vitest';
import { resetBoard } from '../mocks/handlers';
import { server } from '../mocks/server';

beforeAll(() => server.listen({ onUnhandledRequest: 'error' }));
afterAll(() => server.close());

// --- jsdom gaps ---

const matchMediaState = { prefersDark: false };

export const setPrefersDark = (value: boolean) => {
  matchMediaState.prefersDark = value;
};

Object.defineProperty(window, 'matchMedia', {
  writable: true,
  configurable: true,
  value: (query: string) => ({
    matches: query.includes('prefers-color-scheme: dark') ? matchMediaState.prefersDark : false,
    media: query,
    onchange: null,
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    addListener: vi.fn(),
    removeListener: vi.fn(),
    dispatchEvent: vi.fn(),
  }),
});

class MockResizeObserver {
  observe() {}
  unobserve() {}
  disconnect() {}
}

vi.stubGlobal('ResizeObserver', MockResizeObserver);

Element.prototype.scrollIntoView = vi.fn();
window.scrollTo = vi.fn();

// jsdom throws "Not implemented" on navigation; the restart flow reloads the page.
Object.defineProperty(window, 'location', {
  configurable: true,
  writable: true,
  value: { ...window.location, reload: vi.fn(), assign: vi.fn(), replace: vi.fn() },
});

afterEach(() => {
  server.resetHandlers();
  resetBoard();
  cleanup();
  localStorage.clear();
  sessionStorage.clear();
  matchMediaState.prefersDark = false;
});
