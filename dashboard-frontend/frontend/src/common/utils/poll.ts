/** Calls `ask` now and every `ms` while the page is visible; returns the function that stops it. */
export const pollWhileVisible = (ask: () => void, ms: number): (() => void) => {
    let id: ReturnType<typeof setInterval> | null = null;
    const start = () => {
        if (id === null) id = setInterval(ask, ms);
    };
    const pause = () => {
        if (id !== null) clearInterval(id);
        id = null;
    };
    const onVisibility = () => {
        if (document.visibilityState === 'hidden') {
            pause();
        } else if (id === null) {
            ask();
            start();
        }
    };

    ask();
    if (document.visibilityState !== 'hidden') start();
    document.addEventListener('visibilitychange', onVisibility);
    return () => {
        pause();
        document.removeEventListener('visibilitychange', onVisibility);
    };
};
