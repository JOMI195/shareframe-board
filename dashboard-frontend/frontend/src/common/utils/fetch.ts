const apiBaseUrl = import.meta.env.VITE_APP_PRODUCTION === "False"
    ? import.meta.env.VITE_DEV_API_BASE_URL
    : window.location.origin;

export const fetchWithTimeout = async (
    url: string,
    options: RequestInit = {},
    timeout = 10 * 60 * 1000
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