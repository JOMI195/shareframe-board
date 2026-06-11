import { useEffect, useRef, useState } from 'react';
import { Box, CircularProgress } from '@mui/material';
import { useAppDispatch, useAppSelector } from './store';
import {
  checkAuthStatusThunk,
} from './store/auth/auth.Slice';
import Logo from './common/components/logo';
import { hideLoadingWall, selectLoadingWallState } from './store/loadingWall/loadingWall.Slice';
import LoadingWall from './main/loadingWall/loadingWall';
import { usePiConnection } from './context/piConnection/piConnectionContext';
import { RouterProvider } from 'react-router';
import Routing from './routes/routing';

const App = () => {
  const dispatch = useAppDispatch();

  const { isLoadingWallVisible, hideAfter } = useAppSelector(selectLoadingWallState);
  const { isConnected } = usePiConnection();
  const [initialLoad, setInitialLoad] = useState(true);
  const [authCheckComplete, setAuthCheckComplete] = useState(false);

  useEffect(() => {
    const checkAuth = async () => {
      await dispatch(checkAuthStatusThunk());
      setAuthCheckComplete(true);
    };

    checkAuth();
  }, [dispatch]);

  // Dismiss the loading wall once the board is reachable again. Requiring a
  // false -> true transition keeps the wall up while the board reboots (and
  // avoids dismissing it the moment an update is started while still connected).
  const prevConnected = useRef(false);
  useEffect(() => {
    if (isLoadingWallVisible && isConnected && !prevConnected.current) {
      dispatch(hideLoadingWall());
    }
    prevConnected.current = isConnected;
  }, [isConnected, isLoadingWallVisible, dispatch]);

  // Hard fallback: never let the wall stick past its deadline, even if the
  // board never reports healthy.
  useEffect(() => {
    if (!isLoadingWallVisible || !hideAfter) return;
    const timer = setTimeout(
      () => dispatch(hideLoadingWall()),
      Math.max(0, hideAfter - Date.now()),
    );
    return () => clearTimeout(timer);
  }, [isLoadingWallVisible, hideAfter, dispatch]);

  // Timer to ensure loading screen displays for at least 3 seconds
  useEffect(() => {
    const minLoadTimer = setTimeout(() => {
      if (authCheckComplete) {
        setInitialLoad(false);
      }
    }, 3000); // 3 seconds delay

    return () => clearTimeout(minLoadTimer);
  }, [authCheckComplete]);

  useEffect(() => {
    if (!initialLoad) {
      const authCheckInterval = setInterval(() => {
        dispatch(checkAuthStatusThunk());
      }, 5 * 60 * 1000); // 5 minutes in milliseconds

      return () => clearInterval(authCheckInterval);
    }
  }, [dispatch, initialLoad]);

  useEffect(() => {
    if (!initialLoad) {
      const checkAuth = () => {
        dispatch(checkAuthStatusThunk());
      };

      // Check auth when user interacts with the page
      //window.addEventListener('click', checkAuth);
      //window.addEventListener('keydown', checkAuth);

      // Also check when user returns to the tab/window
      window.addEventListener('focus', checkAuth);

      return () => {
        //window.removeEventListener('click', checkAuth);
        //window.removeEventListener('keydown', checkAuth);
        window.removeEventListener('focus', checkAuth);
      };
    }
  }, [dispatch, initialLoad]);

  if (isLoadingWallVisible) {
    return (
      <LoadingWall />
    );
  }

  if (initialLoad) {
    return (
      <Box
        display="flex"
        flexDirection={"column"}
        justifyContent="center"
        alignItems="center"
        height={"100vh"}
      >
        <Logo
          darkLogoSrc="/logo-dark-full-shareframe.svg"
          lightLogoSrc="/logo-light-full-shareframe.svg"
          maxWidth={200}
          marginRight={0}
          clickable={false}
        />
        <CircularProgress size={"50px"} />
      </Box>
    );
  }

  return (
    <RouterProvider
      router={Routing}
    />
  );
}

export default App;