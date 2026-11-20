import React, { useEffect, useRef, useState } from 'react';
import {
  Alert, Box, Stack, Typography, TextField, Button,
  InputAdornment,
  IconButton,
  Link
} from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import { useAppDispatch, useAppSelector } from '@/store';
import { loginThunk, selectAuth } from '@/store/auth/auth.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { removeAllSnackbars } from '@/store/snackbars/snackbars.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useLocation, useNavigate } from 'react-router';
import { getHomeUrl, getNetworkUrl } from '@/assets/endpoints/app/appEndpoints';
import { openGeneralFactoryResetDialog } from '@/store/dialogs/dialogs.Slice';
import FactoryResetDialog from '@/common/components/factoryResetDialog';

type LoginMethod = 'otp' | 'password' | 'setup';

const FIELD: Record<LoginMethod, { name: string; label: string; autoComplete: string }> = {
  otp: { name: 'otp', label: 'Einmal-Code', autoComplete: 'one-time-code' },
  password: { name: 'password', label: 'Passwort', autoComplete: 'current-password' },
  setup: { name: 'setup_code', label: 'Einrichtungscode', autoComplete: 'one-time-code' },
};

const SignIn = () => {
  const dispatch = useAppDispatch();
  const navigate = useNavigate();
  const location = useLocation();
  const { isConnected } = usePiConnection();
  const { isAuthenticated } = useAppSelector(selectAuth);
  const { internet, loaded, mode, ap_ssid } = useAppSelector(selectConnectionMode);
  const [chosenMethod, setMethod] = useState<LoginMethod>('otp');
  const [hotspotMethod, setHotspotMethod] = useState<LoginMethod>('setup');
  // On its own hotspot the frame can't reach the server: the setup code from its panel or the device password.
  const onHotspot = loaded && mode === 'ap';
  const method: LoginMethod = onHotspot ? hotspotMethod : chosenMethod;
  const [secret, setSecret] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(false);
  const [showPassword, setShowPassword] = useState(false);

  // Clears stale errors, but keeps the logout confirmation raised right before.
  const signedOut = (location.state as { signedOut?: boolean } | null)?.signedOut ?? false;
  useEffect(() => {
    if (!signedOut) {
      dispatch(removeAllSnackbars());
    }
  }, [dispatch, signedOut]);

  // Without internet the OTP cannot be verified upstream — default to the
  // offline password once, without ever overriding a manual toggle.
  const autoSwitched = useRef(false);
  useEffect(() => {
    if (!autoSwitched.current && loaded && !internet) {
      autoSwitched.current = true;
      setMethod('password');
    }
  }, [loaded, internet]);

  const handleSubmit = async (e: React.FormEvent): Promise<void> => {
    e.preventDefault();
    setLoading(true);
    try {
      const credentials = method === 'otp' ? { otp: secret }
        : method === 'setup' ? { setup_code: secret.replace(/\s/g, '') }
        : { password: secret };
      const result = await dispatch(loginThunk(credentials));
      if (loginThunk.rejected.match(result)) setSecret('');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isAuthenticated) {
      navigate(onHotspot ? getHomeUrl() + getNetworkUrl() : getHomeUrl());
    }
  }, [isAuthenticated, navigate, onHotspot]);

  const handleToggleMethod = () => {
    if (onHotspot) setHotspotMethod((m) => (m === 'setup' ? 'password' : 'setup'));
    else setMethod((m) => (m === 'otp' ? 'password' : 'otp'));
    setSecret('');
  };
  const otherMethod = method === 'password' ? (onHotspot ? 'Mit Einrichtungscode anmelden' : 'Mit Einmal-Code anmelden')
    : 'Mit Geräte-Passwort anmelden';

  const handleClickShowPassword = () => {
    setShowPassword(!showPassword);
  };

  const handleMouseDownPassword = (
    event: React.MouseEvent<HTMLButtonElement>
  ) => {
    event.preventDefault();
  };

  return (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        textAlign: "center",
        alignItems: 'center',
      }}
    >
      <Typography component="h1" variant="h4" sx={{ mb: 5 }}>
        {"Willkommen bei deinem Bilderrahmen"}
      </Typography>

      <Typography sx={{
        textAlign: "center"
      }}>
        {method === 'otp'
          ? "Melde dich mit einem Einmal-Code an. Du erhältst ihn auf shareframe.de unter „Bilderrahmen“ über „Einmal-Code anzeigen“."
          : method === 'setup'
            ? "Melde dich mit dem Einrichtungscode an. Er steht auf dem Bildschirm des Bilderrahmens."
            : "Melde dich mit dem Geräte-Passwort an."}
      </Typography>

      {onHotspot ? (
        <Alert severity="info" sx={{ mt: 2, width: '100%', textAlign: 'left' }}>
          {`Der Bilderrahmen ist mit keinem WLAN verbunden und stellt den Hotspot „${ap_ssid || 'shareframe-board'}“ bereit. Melde dich an und füge danach unter „Netzwerk“ dein WLAN hinzu.`}
        </Alert>
      ) : loaded && !internet && (
        <Alert severity="info" sx={{ mt: 2, width: '100%' }}>
          {"Keine Internetverbindung. Die Anmeldung mit Einmal-Code ist gerade nicht möglich."}
        </Alert>
      )}

      <Box component="form" onSubmit={handleSubmit} sx={{ width: '100%' }}>
        <TextField
          variant="outlined"
          margin="normal"
          required
          fullWidth
          name={FIELD[method].name}
          label={FIELD[method].label}
          type={method !== 'setup' && !showPassword ? "password" : "text"}
          id={FIELD[method].name}
          autoComplete={FIELD[method].autoComplete}
          helperText={method === 'setup' ? "Siehst du keinen Code, trenne den Rahmen kurz vom Strom." : undefined}
          value={secret}
          onChange={(e) => setSecret(e.target.value)}
          slotProps={{
            htmlInput: method === 'otp'
              ? { autoCapitalize: 'none', autoCorrect: 'off', spellCheck: false }
              : method === 'setup' ? { inputMode: 'numeric' } : undefined,
            input: method === 'setup' ? undefined : {
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    aria-label={`${method === 'otp' ? 'Einmal-Code' : 'Passwort'} ${showPassword ? 'verbergen' : 'anzeigen'}`}
                    onClick={() => handleClickShowPassword()}
                    onMouseDown={handleMouseDownPassword}
                    edge="end"
                  >
                    {showPassword ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              ),
            }
          }}
        />
        <Button
          type="submit"
          fullWidth
          variant="contained"
          color="primary"
          sx={{ mt: 3, mb: 2 }}
          loading={loading}
          disabled={!secret || !isConnected}
        >
          {'Anmelden'}
        </Button>

        <Stack spacing={1} sx={{ alignItems: 'center' }}>
          <Link component="button" type="button" variant="body2" onClick={handleToggleMethod}>
            {otherMethod}
          </Link>
          <Link component="button" type="button" variant="body2"
            onClick={() => dispatch(openGeneralFactoryResetDialog())}>
            {"Passwort vergessen?"}
          </Link>
        </Stack>
      </Box>
      <FactoryResetDialog variant="forgot" />
    </Box>
  );
}

export default SignIn;
