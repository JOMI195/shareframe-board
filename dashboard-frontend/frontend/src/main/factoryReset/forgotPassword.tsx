import { Alert, Box, Button, Divider, Stack, Typography } from '@mui/material';
import { useNavigate } from 'react-router';
import { useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { getAuthenticationUrl, getSignInUrl } from '@/assets/endpoints/app/authEndpoints';
import FactoryResetSteps from './factoryResetSteps';

// Signed out: first the way that keeps everything, then the reset with the code from the panel.
const ForgotPassword = () => {
    const navigate = useNavigate();
    const { mode } = useAppSelector(selectConnectionMode);
    const toSignIn = () => navigate(getAuthenticationUrl() + getSignInUrl(), { replace: true });

    return (
        <Box sx={{ width: '100%' }}>
            <Typography component="h1" variant="h5" gutterBottom sx={{ textAlign: 'center' }}>
                Passwort vergessen?
            </Typography>
            <Stack spacing={2} sx={{ mt: 3 }}>
                <Alert severity="info">
                    {mode === 'ap'
                        ? 'Melde dich mit dem Einrichtungscode vom Bildschirm des Bilderrahmens an und lege unter „System“ ein neues Passwort fest. So bleibt alles erhalten.'
                        : 'Melde dich mit einem Einmal-Code von shareframe.de an und lege unter „System“ ein neues Passwort fest. So bleibt alles erhalten.'}
                </Alert>
                <Button variant="outlined" fullWidth onClick={toSignIn}>Zur Anmeldung</Button>
                <Divider sx={{ pt: 1 }} />
                <Typography component="h2" variant="h6">Auf Werkseinstellungen zurücksetzen</Typography>
                <FactoryResetSteps onLeave={toSignIn} signedIn={false} cancelOnlyOnceStarted />
            </Stack>
        </Box>
    );
};

export default ForgotPassword;
