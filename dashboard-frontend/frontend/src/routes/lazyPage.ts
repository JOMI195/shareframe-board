import type { ComponentType } from 'react';

// Route-level code splitting: the page's chunk loads only when the route matches.
export const lazyPage = (load: () => Promise<{ default: ComponentType }>) => ({
    lazy: async () => ({ Component: (await load()).default }),
});
