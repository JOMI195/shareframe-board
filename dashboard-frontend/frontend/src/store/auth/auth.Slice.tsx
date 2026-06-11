import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { RootState } from '..';
import { IServerResponse } from '@/types';
import uuid from 'react-uuid';
import { addAlertSnackbar, addLoadingSnackbar, removeLoadingSnackbar } from '../snackbars/snackbars.Slice';
import { fetchWithTimeout } from '@/common/utils/fetch';

// Types for Authentication State
interface AuthState {
  isAuthenticated: boolean;
  isLoading: boolean;
  error: string | null;
}

// Initial State
const initialState: AuthState = {
  isAuthenticated: false,
  isLoading: false,
  error: null,
};

// Async Thunk for checking authentication status
export const checkAuthStatusThunk = createAsyncThunk(
  'auth/checkStatus',
  async () => {
    const response = await fetchWithTimeout('/api/auth/status');
    const payload = await response.json();
    return payload?.data?.authenticated ?? false;
  }
);

// Login works with the cloud-verified OTP or, offline, the local device password.
export type LoginCredentials = { otp: string } | { password: string };

// Async Thunk for login
export const loginThunk = createAsyncThunk(
  'auth/login',
  async (credentials: LoginCredentials, { dispatch, rejectWithValue }) => {
    const snackbarId = uuid();
    try {
      // Add loading snackbar
      dispatch(addLoadingSnackbar(
        snackbarId,
        'otp' in credentials ? 'Überprüfung des OTP' : 'Passwort wird überprüft',
      ));

      const response = await fetchWithTimeout('/api/auth/login', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(credentials),
      });

      const data: IServerResponse = await response.json();

      // Remove loading snackbar
      dispatch(removeLoadingSnackbar(snackbarId));

      if (data.success) {
        // Add success snackbar
        dispatch(addAlertSnackbar(
          uuid(),
          'Authentifizierung erfolgreich',
          'success'
        ));
        return true;
      } else {
        // Add error snackbar
        dispatch(addAlertSnackbar(
          uuid(),
          data.message ? data.message : 'Authentifizierung fehlgeschlagen',
          'error'
        ));
        return rejectWithValue(data.message || 'Authentication failed');
      }
    } catch (error) {
      // Remove loading snackbar
      dispatch(removeLoadingSnackbar(snackbarId));

      // Add error snackbar
      dispatch(addAlertSnackbar(
        uuid(),
        'Authentifizierung fehlgeschlagen',
        'error'
      ));

      return rejectWithValue('Authentication failed');
    }
  }
);

// Async Thunk for changing the device password (requires active session)
export const changePasswordThunk = createAsyncThunk(
  'auth/changePassword',
  async (
    { currentPassword, newPassword }: { currentPassword: string; newPassword: string },
    { dispatch, rejectWithValue }
  ) => {
    const snackbarId = uuid();
    try {
      dispatch(addLoadingSnackbar(
        snackbarId,
        'Passwort wird geändert',
      ));

      const response = await fetchWithTimeout('/api/auth/change-password', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          current_password: currentPassword,
          new_password: newPassword,
        }),
      });

      const data: IServerResponse = await response.json();

      dispatch(removeLoadingSnackbar(snackbarId));

      if (data.success) {
        dispatch(addAlertSnackbar(
          uuid(),
          'Passwort erfolgreich geändert',
          'success'
        ));
        return true;
      } else {
        dispatch(addAlertSnackbar(
          uuid(),
          data.message ? data.message : 'Passwortänderung fehlgeschlagen',
          'error'
        ));
        return rejectWithValue(data.message || 'Password change failed');
      }
    } catch (error) {
      dispatch(removeLoadingSnackbar(snackbarId));

      dispatch(addAlertSnackbar(
        uuid(),
        'Passwortänderung fehlgeschlagen',
        'error'
      ));

      return rejectWithValue('Password change failed');
    }
  }
);

// Async Thunk for logout
export const logoutThunk = createAsyncThunk(
  'auth/logout',
  async (_, { dispatch }) => {
    try {
      await fetchWithTimeout('/api/auth/logout', {
        method: 'POST',
      });

      // Add success snackbar
      dispatch(addAlertSnackbar(
        uuid(),
        'Erfolgreich ausgeloggt',
        'success'
      ));

      return true;
    } catch (error) {
      // Add error snackbar
      dispatch(addAlertSnackbar(
        uuid(),
        'Logout fehlgeschlagen',
        'error'
      ));

      throw error;
    }
  }
);

// Slice
export const authSlice = createSlice({
  name: 'auth',
  initialState,
  reducers: {
    resetAuthState: (state) => {
      state.isAuthenticated = false;
      state.isLoading = false;
      state.error = null;
    }
  },
  extraReducers: (builder) => {
    // Check Auth Status
    builder.addCase(checkAuthStatusThunk.pending, (state) => {
      state.isLoading = true;
    });
    builder.addCase(checkAuthStatusThunk.fulfilled, (state, action) => {
      state.isAuthenticated = action.payload;
      state.isLoading = false;
      state.error = null;
    });
    builder.addCase(checkAuthStatusThunk.rejected, (state, action) => {
      state.isAuthenticated = false;
      state.isLoading = false;
      state.error = action.error.message || 'Authentication check failed';
    });

    // Login
    builder.addCase(loginThunk.pending, (state) => {
      state.isLoading = true;
      state.error = null;
    });
    builder.addCase(loginThunk.fulfilled, (state) => {
      state.isAuthenticated = true;
      state.isLoading = false;
      state.error = null;
    });
    builder.addCase(loginThunk.rejected, (state, action) => {
      state.isAuthenticated = false;
      state.isLoading = false;
      state.error = action.payload as string;
    });

    // Logout
    builder.addCase(logoutThunk.pending, (state) => {
      state.isLoading = true;
    });
    builder.addCase(logoutThunk.fulfilled, (state) => {
      state.isAuthenticated = false;
      state.isLoading = false;
      state.error = null;
    });
    builder.addCase(logoutThunk.rejected, (state, action) => {
      state.isLoading = false;
      state.error = action.error.message || 'Logout failed';
    });
  }
});

// Selectors
export const selectAuth = (state: RootState) => state.auth;

export const {
  resetAuthState
} = authSlice.actions;

export default authSlice.reducer;
