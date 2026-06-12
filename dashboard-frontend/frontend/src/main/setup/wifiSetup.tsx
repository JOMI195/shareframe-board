import { useState } from 'react';
import {
    Alert, Box, Button, Card, CardContent, Container, IconButton, InputAdornment,
    Stack, TextField, Typography, useMediaQuery, useTheme
} from '@mui/material';
import Visibility from '@mui/icons-material/Visibility';
import VisibilityOff from '@mui/icons-material/VisibilityOff';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import Logo from '@/common/components/logo';
import NetworkStatusBanner from '@/common/components/networkStatusBanner';
import { useAppDispatch, useAppSelector } from '@/store';
import { addNetwork, selectNetworkState } from '@/store/network/network.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { useNavigate } from 'react-router';
import { getAuthenticationUrl, getSignInUrl } from '@/assets/endpoints/app/authEndpoints';

// AP gateway IP — must match overlay/usr/bin/shareframe-ap (AP_IP) and
// overlay/etc/dnsmasq.d/shareframe-ap.conf on the board.
const AP_GATEWAY_IP = '192.168.4.1';

// Unauthenticated wifi-setup page for AP-fallback mode (no internet, so OTP
// login can't complete). The backend allows /api/connection/* without a session.
const WifiSetup = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const { loading } = useAppSelector(selectNetworkState);
    const { mode, ap_ssid, ap_password } = useAppSelector(selectConnectionMode);

    const [ssid, setSsid] = useState('');
    const [password, setPassword] = useState('');
    const [showPassword, setShowPassword] = useState(false);
    const [submitted, setSubmitted] = useState(false);

    const canSubmit = ssid.trim().length > 0 && password.length >= 8 && !loading;

    const handleSubmit = async () => {
        try {
            await dispatch(addNetwork({ ssid, password })).unwrap();
            setSubmitted(true);
        } catch {
            // Errors are surfaced via snackbars in the thunk.
        }
    };

    return (
        <>
            <NetworkStatusBanner />
            <Container
                component="main"
                maxWidth="xs"
                disableGutters
                sx={{ px: isSmallScreen ? 2 : 0, py: isSmallScreen ? 2 : 5 }}
            >
                <Stack spacing={0} alignItems="center" sx={{ mt: 4, mb: 6, width: '100%' }}>
                    <Logo
                        darkLogoSrc="/logo-dark-full-shareframe.svg"
                        lightLogoSrc="/logo-light-full-shareframe.svg"
                        clickable={false}
                        maxWidth={260}
                    />
                </Stack>

                <Card elevation={1}>
                    <CardContent sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
                        {submitted ? (
                            <Stack spacing={2} alignItems="center" sx={{ py: 2, textAlign: 'center' }}>
                                <CheckCircleIcon color="success" sx={{ fontSize: 48 }} />
                                <Typography variant="h6">WLAN gespeichert</Typography>
                                <Typography variant="body2" color="text.secondary">
                                    Das Gerät versucht jetzt, sich mit „{ssid}“ zu verbinden. Dabei
                                    wird dieser „shareframe-board“-Hotspot beendet und deine Verbindung
                                    trennt sich. Klappt die Verbindung nicht, erscheint der Hotspot
                                    erneut. Du kannst das Gerät alternativ neu starten.
                                </Typography>
                            </Stack>
                        ) : (
                            <>
                                <Typography variant="h6" gutterBottom>
                                    WLAN einrichten
                                </Typography>
                                <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                                    Der Bilderrahmen konnte kein bekanntes WLAN finden. Gib hier deine
                                    Zugangsdaten ein, damit er sich wieder mit dem Internet verbinden kann.
                                </Typography>

                                <TextField
                                    autoFocus
                                    margin="dense"
                                    label="Netzwerkname (SSID)"
                                    type="text"
                                    fullWidth
                                    variant="outlined"
                                    value={ssid}
                                    onChange={(e) => setSsid(e.target.value)}
                                />
                                <TextField
                                    margin="dense"
                                    label="Passwort"
                                    type={showPassword ? 'text' : 'password'}
                                    fullWidth
                                    variant="outlined"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                    helperText="Mindestens 8 Zeichen"
                                    InputProps={{
                                        endAdornment: (
                                            <InputAdornment position="end">
                                                <IconButton
                                                    aria-label="Passwort anzeigen"
                                                    onClick={() => setShowPassword((s) => !s)}
                                                    onMouseDown={(e) => e.preventDefault()}
                                                    edge="end"
                                                >
                                                    {showPassword ? <VisibilityOff /> : <Visibility />}
                                                </IconButton>
                                            </InputAdornment>
                                        ),
                                    }}
                                />

                                <Box sx={{ mt: 2 }}>
                                    <Button
                                        variant="contained"
                                        fullWidth
                                        disabled={!canSubmit}
                                        onClick={handleSubmit}
                                    >
                                        Verbinden
                                    </Button>
                                </Box>

                                <Button
                                    variant="text"
                                    fullWidth
                                    sx={{ mt: 1 }}
                                    onClick={() => navigate(getAuthenticationUrl() + getSignInUrl())}
                                >
                                    Stattdessen mit Passwort anmelden
                                </Button>
                            </>
                        )}
                    </CardContent>
                </Card>

                {mode === 'ap' && (
                    <Alert severity="info" sx={{ mt: 2 }}>
                        Falls sich diese Seite schließt: verbinde dich erneut mit dem WLAN
                        {' '}<b>„{ap_ssid || 'shareframe-board'}“</b>
                        {ap_password ? <> (Passwort: <b>{ap_password}</b>)</> : null} und öffne
                        {' '}<b>http://{AP_GATEWAY_IP}</b> im Browser.
                    </Alert>
                )}
            </Container>
        </>
    );
};

export default WifiSetup;
