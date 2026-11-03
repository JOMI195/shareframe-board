import { useEffect, useState } from 'react';

const DELAY_MS = 600;

// Turns true only once loading has lasted the delay, so fast requests show nothing.
export const useDelayedLoading = (loading: boolean, delayMs = DELAY_MS) => {
    const [delayPassed, setDelayPassed] = useState(false);

    useEffect(() => {
        if (!loading) return;

        const timer = setTimeout(() => setDelayPassed(true), delayMs);

        return () => {
            clearTimeout(timer);
            setDelayPassed(false);
        };
    }, [loading, delayMs]);

    return loading && delayPassed;
};
