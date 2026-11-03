import Button from "@mui/material/Button";
import Box from "@mui/material/Box";
import Typography from "@mui/material/Typography";
import Grid from "@mui/material/Grid";
import { getAuthenticationUrl, getSignInUrl } from "@/assets/endpoints/app/authEndpoints";
import { logoutThunk } from "@/store/auth/auth.Slice";
import { useLocation, useNavigate } from "react-router";
import { useState } from "react";
import { getHomeUrl } from "@/assets/endpoints/app/appEndpoints";
import { useAppDispatch } from "@/store";

const SignOut = () => {
  const dispatch = useAppDispatch();
  const navigate = useNavigate();
  const location = useLocation();
  const [busy, setBusy] = useState(false);

  const handleSignOut = async () => {
    setBusy(true);
    await dispatch(logoutThunk());
    navigate(getAuthenticationUrl() + getSignInUrl(), { replace: true, state: { signedOut: true } });
  };

  // Opened directly there is no page to go back to.
  const handleCancel = () => {
    if (location.key === 'default') navigate(getHomeUrl(), { replace: true });
    else navigate(-1);
  };

  return (
    <Box
      sx={{
        textAlign: "center"
      }}
    >
      <Typography component="h1" variant="h4" sx={{ mb: 5 }}>
        {"Abmelden"}
      </Typography>
      <Box
        component="form"
        id="sign-out-form"
        noValidate
        sx={{ mt: 3 }}
      >
        <Grid container spacing={2}>
          <Grid size={12}>
            <Typography variant="body1" sx={{
              textAlign: "center"
            }}>
              {"Möchtest du dich abmelden?"}
            </Typography>
          </Grid>
          <Grid size={12}>
            <Button
              type="button"
              form="sign-out-form"
              fullWidth
              variant="contained"
              color="primary"
              onClick={handleSignOut}
              loading={busy}
            >
              {"Abmelden"}
            </Button>
          </Grid>
          <Grid size={12}>
            <Button
              type="button"
              fullWidth
              variant="outlined"
              color="secondary"
              onClick={handleCancel}
              disabled={busy}
            >
              {"Abbrechen"}
            </Button>
          </Grid>
        </Grid>
      </Box>
    </Box>
  );
}

export default SignOut;