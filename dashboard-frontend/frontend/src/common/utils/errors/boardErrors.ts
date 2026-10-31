// The board answers partly in English, so only known messages reach the user, in German.
const KNOWN_MESSAGES: Record<string, string> = {
    'OTP invalid': 'Der Einmal-Code stimmt nicht oder ist abgelaufen.',
    'OTP verification failed': 'Der Einmal-Code konnte gerade nicht geprüft werden. Versuche es gleich noch einmal.',
    'Passwort ungültig': 'Das Passwort stimmt nicht.',
    'Aktuelles Passwort ungültig': 'Das aktuelle Passwort stimmt nicht.',
    'Zu viele Fehlversuche. Bitte warte kurz und versuche es erneut.': 'Zu viele Fehlversuche. Bitte warte kurz und versuche es erneut.',
    'Ungültiges AP-Passwort (8–63 ASCII-Zeichen)': 'Das Hotspot-Passwort braucht 8 bis 63 ASCII-Zeichen.',
    'Invalid SSID (max 32 bytes, no control chars)': 'Der Netzwerkname ist ungültig. Er darf höchstens 32 Zeichen haben.',
    'Invalid password (8-63 chars, no control chars)': 'Das WLAN-Passwort braucht 8 bis 63 Zeichen.',
    'Invalid network name': 'Der Netzwerkname ist ungültig.',
    'Not in access point mode': 'Das geht nur, solange der Bilderrahmen seinen Hotspot bereitstellt.',
    'Update service not reachable': 'Der Update-Dienst ist gerade nicht erreichbar.',
    'Failed to check for updates': 'Die Suche nach Updates ist gerade nicht möglich.',
    'Picture not found': 'Das Foto ist nicht mehr auf dem Bilderrahmen.',
    'Picture file missing': 'Das Foto ist nicht mehr auf dem Bilderrahmen.',
};

export const OFFLINE_MESSAGE = 'Du bist offline. Prüfe die WLAN-Verbindung deines Geräts.';
export const UNREACHABLE_MESSAGE = 'Der Bilderrahmen ist gerade nicht erreichbar.';

// A server error keeps the caller's text, which says what failed.
const statusMessage = (status: number): string | undefined => {
    if (status === 429) return 'Zu viele Versuche. Bitte warte kurz.';
    if (status === 413) return 'Die Datei ist zu groß.';
    if (status === 404) return 'Das gibt es nicht mehr.';
    if (status === 403) return 'Dafür fehlt dir die Berechtigung.';
    return undefined;
};

/** Text for a failed board answer: a known board message, else a telling HTTP status, else the caller's own text. */
export const boardErrorMessage = (fallback: string, response?: { status: number }, message?: string) =>
    (message && KNOWN_MESSAGES[message]) ?? (response && statusMessage(response.status)) ?? fallback;

/** Text for a request that got no answer at all. */
export const unreachableMessage = () => (navigator.onLine === false ? OFFLINE_MESSAGE : UNREACHABLE_MESSAGE);
