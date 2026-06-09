import { useState } from 'react';
import {
    Box, Button, Card, CardContent, Container, IconButton, InputAdornment,
    Stack, TextField, Typography, useMediaQuery, useTheme
} from '@mui/material';
import Visibility from '@mui/icons-material/Visibility';
import VisibilityOff from '@mui/icons-material/VisibilityOff';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import Logo from '@/common/components/logo';
import NetworkStatusBanner from '@/common/components/networkStatusBanner';
import { useAppDispatch, useAppSelector } from '@/store';
import { addNetwork, selectNetworkState } from '@/store/network/network.Slice';

// Unauthenticated WiFi-setup page shown while the board hosts its AP fallback.
// In AP mode the board has no internet, so the normal OTP login cannot complete
// — this page lets the user enter new credentials over the board's own AP. The
// backend allows /api/connection/* without a session while in AP mode.
const WifiSetup = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const { loading } = useAppSelector(selectNetworkState);

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
                            </>
                        )}
                    </CardContent>
                </Card>
            </Container>
        </>
    );
};

export default WifiSetup;
