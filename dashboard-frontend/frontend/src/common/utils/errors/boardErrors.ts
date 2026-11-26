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
    'Active network': 'Mit diesem WLAN ist der Bilderrahmen gerade verbunden, deshalb kann es nicht entfernt werden.',
    'Protected network': 'Dieses WLAN lässt sich hier nicht ändern.',
    'Not in access point mode': 'Das geht nur, solange der Bilderrahmen seinen Hotspot bereitstellt.',
    'Update service not reachable': 'Der Update-Dienst ist gerade nicht erreichbar.',
    'Failed to check for updates': 'Die Suche nach Updates ist gerade nicht möglich.',
    'Picture not found': 'Das Foto ist nicht mehr auf dem Bilderrahmen.',
    'Picture file missing': 'Das Foto ist nicht mehr auf dem Bilderrahmen.',
    'Not a JPEG or PNG picture': 'Das Bild lässt sich nicht lesen. Versuche es mit einem anderen Foto.',
    'Picture too large': 'Das Bild ist zu groß.',
    'Picture already there': 'Dieses Bild ist schon ein Standardbild.',
    'Picture too similar': 'Auf dem Display sähe dieses Bild fast genauso aus wie ein vorhandenes Standardbild. Wähle ein deutlich anderes Motiv.',
    'Too many default images': 'Es sind schon so viele Standardbilder wie möglich. Entferne zuerst eines.',
    'Too few default images': 'Es braucht mindestens 3 Standardbilder. Füge zuerst ein neues hinzu.',
    'Not enough space': 'Auf dem Bilderrahmen ist nicht mehr genug Platz.',
    'Dashboard service unavailable': 'Der Bilderrahmen startet seine Einstellungen gerade neu. Versuche es gleich noch einmal.',
    'Dashboard service timed out': 'Der Bilderrahmen antwortet gerade nicht. Versuche es gleich noch einmal.',
    'Setup code invalid': 'Der Einrichtungscode stimmt nicht. Er steht auf dem Bildschirm des Bilderrahmens.',
    'An update is running': 'Während ein Update installiert wird, geht das nicht.',
    'The new version is not confirmed yet': 'Der Bilderrahmen prüft gerade ein Update. Versuche es in ein paar Minuten erneut.',
    'Screen unavailable': 'Der Bildschirm des Bilderrahmens ist gerade aus, deshalb kann dort kein Code erscheinen.',
    'Too many reset codes': 'Zu viele Codes angefordert. Versuche es in einer Stunde erneut.',
    'No reset code requested': 'Der Code ist abgelaufen. Fordere einen neuen an.',
    'Cross-origin request refused': 'Diese Anfrage kam nicht von den Einstellungen des Bilderrahmens.',
    'No newer version available': 'Es gibt keine neuere Version.',
    'Update already in progress': 'Ein Update läuft bereits.',
    'Previous update awaits confirmation; reboot or wait first': 'Das letzte Update wird noch bestätigt. Starte den Bilderrahmen neu oder warte ein paar Minuten.',
    'Failed to fetch latest release from server': 'Der Update-Server ist gerade nicht erreichbar.',
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

// The updater reports in English, starting with the step that failed.
const UPDATE_FAILURES: [string, string][] = [
    ['Download failed', 'Das Update konnte nicht heruntergeladen werden. Prüfe die Internetverbindung und versuche es erneut.'],
    ['Checksum mismatch', 'Das heruntergeladene Update war beschädigt. Versuche es erneut.'],
    ['rauc install failed', 'Das Update konnte nicht installiert werden. Der Bilderrahmen läuft mit der bisherigen Version weiter.'],
    ['Installed, but the reboot failed', 'Das Update ist installiert, aber der Neustart hat nicht geklappt. Starte den Bilderrahmen unter „System“ neu.'],
];

/** German text for a failed update; the board's own words go along as details. */
export const updateFailure = (raw: string) =>
    UPDATE_FAILURES.find(([prefix]) => raw.startsWith(prefix))?.[1]
    ?? 'Das Update ist fehlgeschlagen. Versuche es später erneut.';
