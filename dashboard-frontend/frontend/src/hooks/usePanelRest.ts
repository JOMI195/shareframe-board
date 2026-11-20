import { useEffect, useState } from 'react';
import { useAppSelector } from '@/store';
import { selectSlideshowStatus } from '@/store/slideshowStatus/slideshowStatus.Slice';

/** Seconds until the board lets the e-paper panel paint again; 0 when it may now. */
export const usePanelRest = (): number => {
    const { refreshReadyAt, lastCheckedAt } = useAppSelector(selectSlideshowStatus);
    const [now, setNow] = useState(() => Date.now());

    useEffect(() => {
        if (refreshReadyAt === null) return;
        const id = setInterval(() => setNow(Date.now()), 1000);
        return () => clearInterval(id);
    }, [refreshReadyAt]);

    if (refreshReadyAt === null) return 0;
    // The clock here may be older than the status that set the deadline.
    const at = Math.max(now, lastCheckedAt ?? 0);
    return Math.max(0, Math.ceil((refreshReadyAt - at) / 1000));
};

export const formatRest = (secs: number) => `${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, '0')}`;
