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

export const formatTimeLeft = (expiresAt: number, now: number = Date.now()): string => {
    const left = expiresAt * 1000 - now;
    return left <= 0 ? 'Abgelaufen' : `noch ${formatDuration(left)}`;
};
