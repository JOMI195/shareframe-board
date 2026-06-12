import React, { useEffect, useRef, useState } from 'react';
import {
  Alert, Box, Typography, TextField, Button,
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
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useNavigate } from 'react-router';
import { getHomeUrl, getSetupUrl } from '@/assets/endpoints/app/appEndpoints';

type LoginMethod = 'otp' | 'password';

const SignIn = () => {
  const dispatch = useAppDispatch();
  const navigate = useNavigate();
  const { isConnected } = usePiConnection();
  const { isAuthenticated } = useAppSelector(selectAuth);
  const { internet, loaded, mode } = useAppSelector(selectConnectionMode);
  const [method, setMethod] = useState<LoginMethod>('otp');
  const [secret, setSecret] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(false);
  const [showPassword, setShowPassword] = useState(false);

  useEffect(() => {
    dispatch(removeAllSnackbars());
  }, []);

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
      await dispatch(loginThunk(method === 'otp' ? { otp: secret } : { password: secret }));
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isAuthenticated) {
      navigate(getHomeUrl());
    }
  }, [isAuthenticated]);

  const handleToggleMethod = () => {
    setMethod((m) => (m === 'otp' ? 'password' : 'otp'));
    setSecret('');
  };

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

      <Typography textAlign={"center"}>
        {method === 'otp'
          ? "Nutze ein OTP um dich bei deinem Bilderrahmen anzumelden. Dieses erhälst du in der ShareFrame Website Bilderrahmen Übersicht"
          : "Melde dich mit dem Geräte-Passwort an."}
      </Typography>

      {loaded && !internet && (
        <Alert severity="info" sx={{ mt: 2, width: '100%' }}>
          {"Keine Internetverbindung – die OTP-Anmeldung ist derzeit nicht möglich."}
          {mode === 'ap' && (
            <>
              {' '}
              <Link component="button" type="button" onClick={() => navigate(getSetupUrl())}>
                {"WLAN einrichten"}
              </Link>
            </>
          )}
        </Alert>
      )}

      <Box component="form" onSubmit={handleSubmit} sx={{ width: '100%' }}>
        <TextField
          variant="outlined"
          margin="normal"
          required
          fullWidth
          name="password"
          label={method === 'otp' ? "OTP" : "Passwort"}
          type={showPassword ? "text" : "password"}
          id="password"
          autoComplete="current-password"
          value={secret}
          onChange={(e) => setSecret(e.target.value)}
          InputProps={{
            endAdornment: (
              <InputAdornment position="end">
                <IconButton
                  aria-label="toggle current password visibility"
                  onClick={() => handleClickShowPassword()}
                  onMouseDown={handleMouseDownPassword}
                  edge="end"
                >
                  {showPassword ? <VisibilityOff /> : <Visibility />}
                </IconButton>
              </InputAdornment>
            ),
          }}
        />
        <Button
          type="submit"
          fullWidth
          variant="contained"
          color="primary"
          sx={{ mt: 3, mb: 2 }}
          disabled={loading || !secret || !isConnected}
        >
          {'Anmelden'}
        </Button>

        <Link component="button" type="button" variant="body2" onClick={handleToggleMethod}>
          {method === 'otp' ? "Mit Passwort anmelden" : "Mit OTP anmelden"}
        </Link>
      </Box>
    </Box>
  );
}

export default SignIn;
