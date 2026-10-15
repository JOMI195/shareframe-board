const API_BASE = import.meta.env.VITE_DEV_API_BASE_URL;

// Handlers are built from the app's own URL builders, so a path change breaks
// the handler instead of silently passing.
export const apiUrl = (path: string) => `${API_BASE}${path}`;

// MSW matches on the path alone; builders that add a query string go through here.
export const apiPath = (url: string) => apiUrl(url.split('?')[0]);
