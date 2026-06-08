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

// Async Thunk for login
export const loginThunk = createAsyncThunk(
  'auth/login',
  async (otp: string, { dispatch, rejectWithValue }) => {
    const snackbarId = uuid();
    try {
      // Add loading snackbar
      dispatch(addLoadingSnackbar(
        snackbarId,
        'Überprüfung des OTP',
      ));

      const response = await fetchWithTimeout('/api/auth/login', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ otp }),
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
