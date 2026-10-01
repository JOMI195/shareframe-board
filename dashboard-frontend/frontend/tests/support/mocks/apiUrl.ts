const API_BASE = import.meta.env.VITE_DEV_API_BASE_URL;

// Handlers are built from the app's own URL builders, so a path change breaks
// the handler instead of silently passing.
export const apiUrl = (path: string) => `${API_BASE}${path}`;
