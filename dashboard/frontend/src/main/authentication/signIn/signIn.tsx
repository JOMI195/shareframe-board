import React, { useEffect, useState } from 'react';
import {
  Box, Typography, TextField, Button,
  InputAdornment,
  IconButton
} from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import { useAppDispatch, useAppSelector } from '@/store';
import { loginThunk, selectAuth } from '@/store/auth/auth.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useNavigate } from 'react-router';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

const SignIn = () => {
  const dispatch = useAppDispatch();
  const navigate = useNavigate();
  const { isConnected } = usePiConnection();
  const { isAuthenticated } = useAppSelector(selectAuth);
  const [password, setPassword] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(false);
  const [showPassword, setShowPassword] = useState(false);

  const handleSubmit = async (e: React.FormEvent): Promise<void> => {
    e.preventDefault();
    setLoading(true);
    try {
      await dispatch(loginThunk(password));
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (isAuthenticated) {
      navigate(getHomeUrl());
    }
  }, [isAuthenticated]);

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
        {"Nutze ein OTP um dich bei deinem Bilderrahmen anzumelden. Dieses erhälst du in der ShareFrame Website Bilderrahmen Übersicht"}
      </Typography>

      <Box component="form" onSubmit={handleSubmit} sx={{ width: '100%' }}>
        <TextField
          variant="outlined"
          margin="normal"
          required
          fullWidth
          name="password"
          label="OTP"
          type={showPassword ? "text" : "password"}
          id="password"
          autoComplete="current-password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
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
          disabled={loading || !password || !isConnected}
        >
          {'Anmelden'}
        </Button>
      </Box>
    </Box>
  );
}

export default SignIn;
