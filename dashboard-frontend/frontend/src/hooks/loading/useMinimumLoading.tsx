import { useEffect, useRef, useState } from 'react';

const MINIMUM_LOADING_MS = 400;

// Holds a loading flag true for a minimum span so fast responses don't flash the skeleton.
export const useMinimumLoading = (loading: boolean, minimumMs = MINIMUM_LOADING_MS) => {
    const [visible, setVisible] = useState(loading);
    const startedAt = useRef<number | null>(null);

    useEffect(() => {
        if (loading) {
            startedAt.current = Date.now();
            // eslint-disable-next-line react-hooks/set-state-in-effect -- the skeleton must stay mounted across the whole request
            setVisible(true);
            return;
        }

        if (startedAt.current === null) return;

        const remaining = minimumMs - (Date.now() - startedAt.current);
        startedAt.current = null;

        if (remaining <= 0) {
            setVisible(false);
            return;
        }

        const timer = setTimeout(() => setVisible(false), remaining);

        return () => clearTimeout(timer);
    }, [loading, minimumMs]);

    return visible;
};
