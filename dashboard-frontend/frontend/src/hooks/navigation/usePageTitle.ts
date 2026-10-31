import { useEffect } from 'react';
import { useMatches } from 'react-router';

export const usePageTitle = () => {
    const matches = useMatches();
    const title = (matches[matches.length - 1]?.handle as { title?: string } | undefined)?.title;

    useEffect(() => {
        document.title = title ? `${title} – ShareFrame` : 'ShareFrame';
    }, [title]);

    return title;
};
