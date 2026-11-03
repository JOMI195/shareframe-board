const STALE_CHUNK_KEY = 'stale-chunk-reload';
const STALE_CHUNK_MESSAGE = /dynamically imported module|Importing a module script failed|error loading dynamically/i;
const RETRY_AFTER_MS = 30_000;

export const isStaleChunkError = (error: unknown) =>
    error instanceof Error && STALE_CHUNK_MESSAGE.test(error.message);

// Reloads at most every 30 s, so a chunk that is really missing can't loop the page.
export const reloadForStaleChunk = () => {
    const last = Number(sessionStorage.getItem(STALE_CHUNK_KEY) ?? 0);
    if (Date.now() - last < RETRY_AFTER_MS) return false;
    sessionStorage.setItem(STALE_CHUNK_KEY, String(Date.now()));
    window.location.reload();
    return true;
};

// An update replaces the hashed chunks, so an open tab can only recover by loading the new build.
export const initStaleChunkReload = () => {
    window.addEventListener('vite:preloadError', (event) => {
        if (reloadForStaleChunk()) event.preventDefault();
    });
};
