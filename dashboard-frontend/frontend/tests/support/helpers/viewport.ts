import { vi } from 'vitest';

// The setup mock answers min-width queries with false, which is the phone layout.
export const desktopViewport = () => {
  const matchMedia = window.matchMedia;
  vi.spyOn(window, 'matchMedia').mockImplementation((query: string) => ({
    ...matchMedia(query),
    matches: query.includes('min-width'),
  }));
};

export const floatingButton = () => document.querySelector('.MuiFab-root');
