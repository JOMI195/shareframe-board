import { formatGermanDateTime } from '@/common/components/dateUtils';

const MINUTE_MS = 60 * 1000;

const plural = (count: number, singular: string, pluralForm: string) =>
    `${count} ${count === 1 ? singular : pluralForm}`;

export const formatDuration = (durationMs: number): string => {
    const totalMinutes = Math.round(durationMs / MINUTE_MS);
    const days = Math.floor(totalMinutes / (24 * 60));
    const hours = Math.floor((totalMinutes % (24 * 60)) / 60);
    const minutes = totalMinutes % 60;
    const parts = [
        days > 0 && plural(days, 'Tag', 'Tage'),
        hours > 0 && plural(hours, 'Stunde', 'Stunden'),
        minutes > 0 && plural(minutes, 'Minute', 'Minuten'),
    ].filter(Boolean);
    return parts.length > 0 ? parts.join(' ') : 'weniger als 1 Minute';
};

export const formatUnix = (seconds: number | null): string =>
    seconds === null ? '–' : formatGermanDateTime(new Date(seconds * 1000));

// The board stores a picture the server sent without an expiry with 0.
export const NEVER_EXPIRES = 0;

export const formatExpiresAt = (expiresAt: number): string =>
    expiresAt === NEVER_EXPIRES ? 'Nie' : formatUnix(expiresAt);

export const formatTimeLeft = (expiresAt: number, now: number = Date.now()): string => {
    if (expiresAt === NEVER_EXPIRES) return 'Läuft nicht ab';
    const left = expiresAt * 1000 - now;
    return left <= 0 ? 'Abgelaufen' : `noch ${formatDuration(left)}`;
};
