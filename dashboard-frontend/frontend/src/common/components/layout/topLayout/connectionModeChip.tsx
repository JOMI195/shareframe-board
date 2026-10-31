import { Chip, Tooltip } from '@mui/material';
import WifiIcon from '@mui/icons-material/Wifi';
import WifiFindIcon from '@mui/icons-material/WifiFind';
import WifiTetheringIcon from '@mui/icons-material/WifiTethering';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import { useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';

const ConnectionModeChip = () => {
    const { mode, ssid, internet, ap_ssid, loaded } = useAppSelector(selectConnectionMode);
    if (!loaded) return null;

    const [label, hint, color, Icon] =
        mode === 'ap' ? ['Hotspot', `Rahmen stellt den Hotspot „${ap_ssid || 'shareframe-board'}“ bereit`, 'error' as const, WifiTetheringIcon]
        : mode === 'connecting' ? ['Verbinde …', 'Rahmen sucht ein gespeichertes WLAN', 'warning' as const, WifiFindIcon]
        : internet ? ['Online', ssid ? `Mit „${ssid}“ verbunden` : 'Mit dem Internet verbunden', 'success' as const, WifiIcon]
        : ['Kein Internet', ssid ? `Mit „${ssid}“ verbunden, aber ohne Internet` : 'Keine Internetverbindung', 'warning' as const, SignalWifiStatusbarConnectedNoInternet4Icon];

    return (
        <Tooltip title={hint}>
            <Chip
                size="small"
                variant="outlined"
                color={color}
                icon={<Icon />}
                label={label}
                aria-label={`Verbindung: ${label}`}
                sx={{ mr: 1 }}
            />
        </Tooltip>
    );
};

export default ConnectionModeChip;
