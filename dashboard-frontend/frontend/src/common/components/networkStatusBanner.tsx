import { Box, Typography } from '@mui/material';
import WifiTetheringIcon from '@mui/icons-material/WifiTethering';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import { useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';

// Sticky network-mode banner: hidden on the happy path, shown for connecting /
// no-internet / AP mode. Polling is central (RouterContext); this only reads.
const NetworkStatusBanner = () => {
    const { mode, ssid, internet, ap_ssid, ap_password, loaded } = useAppSelector(selectConnectionMode);

    if (!loaded) return null;
    if (mode === 'connected' && internet) return null;

    let bgcolor = 'warning.main';
    let Icon = SignalWifiStatusbarConnectedNoInternet4Icon;
    let title = '';
    let detail = '';

    if (mode === 'ap') {
        bgcolor = 'error.main';
        Icon = WifiTetheringIcon;
        title = 'AP-Modus aktiv';
        detail = `WLAN „${ap_ssid || 'shareframe-board'}“${ap_password ? ` · Passwort: ${ap_password}` : ''} — neues WLAN hinzufügen, das Gerät verbindet sich dann neu (oder neu starten).`;
    } else if (mode === 'connecting') {
        bgcolor = 'warning.main';
        title = 'Verbinde…';
        detail = ssid ? `Verbindung zu „${ssid}“ wird hergestellt.` : 'Suche nach bekannten Netzwerken.';
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
                <Typography variant="body2" fontWeight={600} lineHeight={1.2}>
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
