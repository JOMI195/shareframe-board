import { Alert, AlertTitle } from '@mui/material';
import WifiTetheringIcon from '@mui/icons-material/WifiTethering';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import CloudOffIcon from '@mui/icons-material/CloudOff';
import { useLocation } from 'react-router';
import { useAppSelector } from '@/store';
import { getHomeUrl, getNetworkUrl } from '@/assets/endpoints/app/appEndpoints';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { formatDuration } from '@/main/slideshowImages/slideshowImagesUtils';

// A reconnect normally takes seconds; only a longer gap is worth a banner.
const SERVER_GRACE_SECS = 120;

// Network-mode notice: hidden on the happy path, shown for connecting / no internet /
// hotspot / server unreachable. Polling is central (RouterContext); this only reads.
const NetworkStatusBanner = () => {
    const { mode, ssid, internet, ap_ssid, server_connected, server_offline_secs, server_last_contact_at, loaded } = useAppSelector(selectConnectionMode);
    const { pathname } = useLocation();

    if (!loaded) return null;
    const serverDown = server_connected === false && (server_last_contact_at === 0 || server_offline_secs > SERVER_GRACE_SECS);
    if (mode === 'connected' && internet && !serverDown) return null;
    // There the "WLAN einrichten" card says the same and has the button.
    if (mode === 'ap' && pathname === getHomeUrl() + getNetworkUrl()) return null;

    let severity: 'info' | 'warning' = 'warning';
    let Icon = SignalWifiStatusbarConnectedNoInternet4Icon;
    let title: string;
    let detail: string;

    if (mode === 'ap') {
        // Expected while setting the frame up, so information rather than a warning.
        severity = 'info';
        Icon = WifiTetheringIcon;
        title = 'Hotspot aktiv';
        detail = `Der Bilderrahmen stellt den Hotspot „${ap_ssid || 'shareframe-board'}“ bereit. Füge unter „Netzwerk“ ein WLAN hinzu und wähle „Jetzt verbinden“.`;
    } else if (mode === 'connecting') {
        title = 'Verbindet …';
        detail = ssid ? `Verbindung zu „${ssid}“ wird hergestellt.` : 'Suche nach bekannten Netzwerken.';
    } else if (internet) {
        Icon = CloudOffIcon;
        title = 'Server nicht erreichbar';
        detail = server_last_contact_at > 0
            ? `Letzter Kontakt vor ${formatDuration(server_offline_secs * 1000)}. Neue Fotos kommen an, sobald die Verbindung wieder steht.`
            : 'Noch keine Verbindung zum Server. Neue Fotos kommen an, sobald die Verbindung steht.';
    } else {
        title = 'Kein Internet';
        detail = ssid ? `Mit „${ssid}“ verbunden, aber keine Internetverbindung.` : 'Keine Internetverbindung.';
    }

    // In the page flow like the main app's notices: it scrolls with the page and covers nothing.
    return (
        <Alert role="status" severity={severity} icon={<Icon fontSize="inherit" />} sx={{ width: '100%', mb: 2 }}>
            <AlertTitle sx={{ mb: 0.25 }}>{title}</AlertTitle>
            {detail}
        </Alert>
    );
};

export default NetworkStatusBanner;
