const apiBaseUrl = import.meta.env.VITE_APP_PRODUCTION === "False"
    ? import.meta.env.VITE_DEV_API_BASE_URL
    : window.location.origin;

// Most answers take a second; the calls that wait on a script, the server or a download pass their own.
export const DEFAULT_TIMEOUT_MS = 15_000;
export const SLOW_TIMEOUT_MS = 60_000;
// A board script runs up to 20 s (saving a network, the hotspot password, a one-time code check).
export const SCRIPT_TIMEOUT_MS = 30_000;

export const fetchWithTimeout = async (
    url: string,
    options: RequestInit = {},
    timeout = DEFAULT_TIMEOUT_MS
): Promise<Response> => {
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);

    try {
        const response = await fetch(`${apiBaseUrl}${url}`, {
            ...options,
            credentials: 'include',
            signal: controller.signal
        });
        clearTimeout(id);
        return response;
    } catch (error) {
        clearTimeout(id);
        throw error;
    }
};