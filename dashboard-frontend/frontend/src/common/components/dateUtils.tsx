export function formatGermanDateTime(date: Date): string {
    const newDate = new Date(date);
    return newDate.toLocaleString('de-DE', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
    });
}
