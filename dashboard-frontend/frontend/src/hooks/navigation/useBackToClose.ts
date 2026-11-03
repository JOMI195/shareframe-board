import { useEffect, useRef, useState } from 'react';

const MARKER = 'backToClose';

/**
 * Lets the browser and Android back button close an overlay instead of leaving the page.
 * Opening pushes a marker entry on the same URL; closing in any other way removes it again.
 */
export const useBackToClose = (open: boolean, onClose: () => void) => {
    const onCloseRef = useRef(onClose);
    // Bumped on every back press, so an overlay that refuses to close (e.g. asks first) gets a new marker.
    const [backPresses, setBackPresses] = useState(0);

    useEffect(() => {
        onCloseRef.current = onClose;
    });

    useEffect(() => {
        if (!open) return;

        const id = `${Date.now()}-${Math.random()}`;
        window.history.pushState({ ...window.history.state, [MARKER]: id }, '');
        let poppedByBack = false;

        const onPopState = () => {
            // Landing on our own marker means an overlay opened above this one went away.
            if (window.history.state?.[MARKER] === id) return;
            poppedByBack = true;
            window.removeEventListener('popstate', onPopState);
            onCloseRef.current();
            setBackPresses((count) => count + 1);
        };
        window.addEventListener('popstate', onPopState);

        return () => {
            window.removeEventListener('popstate', onPopState);
            if (!poppedByBack && window.history.state?.[MARKER] === id) window.history.back();
        };
    }, [open, backPresses]);
};
