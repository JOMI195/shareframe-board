import { Box, Typography } from '@mui/material';
import WifiTetheringIcon from '@mui/icons-material/WifiTethering';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import CloudOffIcon from '@mui/icons-material/CloudOff';
import { useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { formatDuration } from '@/main/slideshowImages/slideshowImagesUtils';

// A reconnect normally takes seconds; only a longer gap is worth a banner.
const SERVER_GRACE_SECS = 120;

// Sticky network-mode banner: hidden on the happy path, shown for connecting /
// no-internet / AP mode / server unreachable. Polling is central (RouterContext); this only reads.
const NetworkStatusBanner = () => {
    const { mode, ssid, internet, ap_ssid, ap_password, server_connected, server_offline_secs, server_last_contact_at, loaded } = useAppSelector(selectConnectionMode);

    if (!loaded) return null;
    const serverDown = server_connected === false && (server_last_contact_at === 0 || server_offline_secs > SERVER_GRACE_SECS);
    if (mode === 'connected' && internet && !serverDown) return null;

    let bgcolor: string;
    let Icon = SignalWifiStatusbarConnectedNoInternet4Icon;
    let title: string;
    let detail: string;

    if (mode === 'ap') {
        bgcolor = 'error.main';
        Icon = WifiTetheringIcon;
        title = 'AP-Modus aktiv';
        detail = `WLAN „${ap_ssid || 'shareframe-board'}“${ap_password ? ` · Passwort: ${ap_password}` : ''} — unter Netzwerk ein WLAN hinzufügen und „Jetzt verbinden“ wählen.`;
    } else if (mode === 'connecting') {
        bgcolor = 'warning.main';
        title = 'Verbinde…';
        detail = ssid ? `Verbindung zu „${ssid}“ wird hergestellt.` : 'Suche nach bekannten Netzwerken.';
    } else if (internet) {
        bgcolor = 'warning.main';
        Icon = CloudOffIcon;
        title = 'Server nicht erreichbar';
        detail = server_last_contact_at > 0
            ? `Letzter Kontakt vor ${formatDuration(server_offline_secs * 1000)}. Neue Fotos kommen an, sobald die Verbindung wieder steht.`
            : 'Noch keine Verbindung zum Server. Neue Fotos kommen an, sobald die Verbindung steht.';
    } else {
        // connected but no internet
        bgcolor = 'warning.main';
        title = 'Kein Internet';
        detail = ssid ? `Mit „${ssid}“ verbunden, aber keine Internetverbindung.` : 'Keine Internetverbindung.';
    }

    return (
        <Box
            role="status"
            sx={{
                position: 'sticky',
                top: theme => theme.layout.appbar.height,
                zIndex: theme => theme.zIndex.appBar - 1,
                bgcolor,
                color: '#fff',
                px: 2,
                py: 1,
                display: 'flex',
                alignItems: 'center',
                gap: 1.5,
            }}
        >
            <Icon fontSize="small" />
            <Box sx={{ minWidth: 0 }}>
                <Typography
                    variant="body2"
                    sx={{
                        fontWeight: 600,
                        lineHeight: 1.2
                    }}>
                    {title}
                </Typography>
                <Typography variant="caption" sx={{ opacity: 0.95 }}>
                    {detail}
                </Typography>
            </Box>
        </Box>
    );
};

export default NetworkStatusBanner;
