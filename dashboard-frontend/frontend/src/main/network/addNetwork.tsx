import { useEffect, useState } from 'react';
import {
    Alert, Button, Checkbox, FormControlLabel, IconButton, InputAdornment, List, ListItemButton, ListItemIcon,
    ListItemText, Stack, TextField, Typography
} from '@mui/material';
import { useNavigate } from 'react-router';
import Visibility from '@mui/icons-material/Visibility';
import VisibilityOff from '@mui/icons-material/VisibilityOff';
import LockIcon from '@mui/icons-material/Lock';
import SignalWifi1BarIcon from '@mui/icons-material/SignalWifi1Bar';
import SignalWifi2BarIcon from '@mui/icons-material/SignalWifi2Bar';
import SignalWifi3BarIcon from '@mui/icons-material/SignalWifi3Bar';
import SignalWifi4BarIcon from '@mui/icons-material/SignalWifi4Bar';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import { getHomeUrl, getNetworkUrl } from '@/assets/endpoints/app/appEndpoints';
import {
    addNetwork, connectToSavedNetworks, fetchNetworksInReach, NetworkInReach, selectNetworkState
} from '@/store/network/network.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { boardAddress } from '@/common/utils/boardAddress';

// What wpa_supplicant takes: an SSID of up to 32 bytes, a WPA passphrase of 8 to 63 characters.
const SSID_MAX_BYTES = 32;
const PASSWORD_MIN = 8;
const PASSWORD_MAX = 63;

const SignalIcon = ({ signal }: { signal: number }) => {
    const Icon = signal >= -55 ? SignalWifi4BarIcon : signal >= -67 ? SignalWifi3BarIcon
        : signal >= -75 ? SignalWifi2BarIcon : SignalWifi1BarIcon;
    return <Icon fontSize="small" />;
};

// A page rather than a dialog: on the hotspot saving also ends this page's connection.
const AddNetwork = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();
    const { loading, inReach } = useAppSelector(selectNetworkState);
    const { mode, ap_ssid, hostname } = useAppSelector(selectConnectionMode);
    const [ssid, setSsid] = useState('');
    const [password, setPassword] = useState('');
    const [openNetwork, setOpenNetwork] = useState(false);
    const [showPassword, setShowPassword] = useState(false);
    const [submitting, setSubmitting] = useState(false);
    const onHotspot = mode === 'ap';

    useEffect(() => {
        dispatch(fetchNetworksInReach());
    }, [dispatch]);

    const ssidTooLong = new TextEncoder().encode(ssid).length > SSID_MAX_BYTES;
    const passwordFits = password.length >= PASSWORD_MIN && password.length <= PASSWORD_MAX;
    const canSubmit = ssid.trim() !== '' && !ssidTooLong && (openNetwork || passwordFits) && !loading && isConnected;

    const backToNetwork = () => navigate(getHomeUrl() + getNetworkUrl(), { replace: true });

    const pick = (network: NetworkInReach) => {
        setSsid(network.ssid);
        setOpenNetwork(!network.secured);
        setPassword('');
    };

    const submit = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!canSubmit) return;
        setSubmitting(true);
        try {
            await dispatch(addNetwork({ ssid, password: openNetwork ? '' : password })).unwrap();
            // On the hotspot the new network is only useful once the frame switches over.
            if (onHotspot) await dispatch(connectToSavedNetworks()).unwrap();
            backToNetwork();
        } catch {
            // The thunks report the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const form = (
        <Stack component="form" spacing={2} onSubmit={submit} noValidate>
            {inReach.length > 0 && (
                <>
                    <Typography variant="subtitle2" component="h3">In der Nähe</Typography>
                    <List dense disablePadding sx={{ maxHeight: 240, overflowY: 'auto' }}>
                        {inReach.slice(0, 10).map((network) => (
                            <ListItemButton
                                key={network.ssid}
                                selected={network.ssid === ssid}
                                onClick={() => pick(network)}
                                sx={{ borderRadius: 1 }}
                            >
                                <ListItemIcon sx={{ minWidth: 36 }}><SignalIcon signal={network.signal} /></ListItemIcon>
                                <ListItemText primary={network.ssid} slotProps={{ primary: { noWrap: true } }} />
                                {network.secured && <LockIcon fontSize="small" titleAccess="geschützt" sx={{ color: 'text.secondary' }} />}
                            </ListItemButton>
                        ))}
                    </List>
                </>
            )}
            {/* No autofocus: a phone's keyboard would cover the networks in reach. */}
            <TextField
                id="ssid"
                label="Netzwerkname (SSID)"
                type="text"
                autoComplete="off"
                autoCapitalize="none"
                fullWidth
                value={ssid}
                onChange={(e) => setSsid(e.target.value)}
                error={ssidTooLong}
                helperText={ssidTooLong ? 'Höchstens 32 Zeichen' : inReach.length > 0 ? 'Oben auswählen oder eintippen, etwa für ein verstecktes WLAN' : undefined}
                slotProps={{ htmlInput: { autoCorrect: 'off', spellCheck: false } }}
            />
            <FormControlLabel
                control={<Checkbox checked={openNetwork} onChange={(e) => setOpenNetwork(e.target.checked)} />}
                label="Offenes WLAN ohne Passwort"
            />
            {!openNetwork && (
                <TextField
                    id="password"
                    label="Passwort"
                    type={showPassword ? 'text' : 'password'}
                    autoComplete="off"
                    fullWidth
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    error={password.length > 0 && !passwordFits}
                    helperText="8 bis 63 Zeichen"
                    slotProps={{
                        input: {
                            endAdornment: (
                                <InputAdornment position="end">
                                    <IconButton
                                        aria-label={showPassword ? 'Passwort verbergen' : 'Passwort anzeigen'}
                                        onClick={() => setShowPassword(!showPassword)}
                                        onMouseDown={(e) => e.preventDefault()}
                                        edge="end"
                                    >
                                        {showPassword ? <VisibilityOff /> : <Visibility />}
                                    </IconButton>
                                </InputAdornment>
                            ),
                        },
                    }}
                />
            )}
            {onHotspot && (
                <Alert severity="info">
                    Danach beendet der Rahmen den Hotspot und verbindet sich, dein Gerät verliert diese Seite.
                    Klappt es, öffne in deinem WLAN <b>{boardAddress(hostname, ap_ssid)}</b>. Sonst ist der
                    Hotspot nach etwa einer Minute wieder da.
                </Alert>
            )}
            {/* Stacked on a phone, on the right in a wide card. */}
            <Stack
                direction={{ xs: 'column', sm: 'row-reverse' }}
                sx={{ gap: 1, '& > .MuiButton-root': { width: { xs: '100%', sm: 'auto' } } }}
            >
                <Button type="submit" variant="contained" loading={submitting} disabled={!canSubmit}>
                    {onHotspot ? 'Speichern & verbinden' : 'Hinzufügen'}
                </Button>
                <Button onClick={backToNetwork} disabled={submitting}>Abbrechen</Button>
            </Stack>
        </Stack>
    );

    return (
        <ShareframeInfoCard
            title="WLAN hinzufügen"
            sx={{ width: '100%' }}
            sections={[{ content: { type: 'reactNode', value: form } }]}
        />
    );
};

export default AddNetwork;
