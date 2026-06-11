import { useAppDispatch, useAppSelector } from "@/store";
import { setCurrentPath } from "@/store/navigation/navigation.Slice";
import { useEffect } from "react";
import { Outlet, useLocation, useNavigate } from "react-router";
import { fetchConnectionMode, selectConnectionMode } from "@/store/connectionMode/connectionMode.Slice";
import { selectAuth } from "@/store/auth/auth.Slice";
import { getSetupUrl } from "@/assets/endpoints/app/appEndpoints";
import { getAuthenticationUrl } from "@/assets/endpoints/app/authEndpoints";

const RouterContext = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const location = useLocation();

    const { mode, loaded } = useAppSelector(selectConnectionMode);
    const { isAuthenticated } = useAppSelector(selectAuth);

    useEffect(() => {
        dispatch(setCurrentPath(location.pathname));
    }, [dispatch, location.pathname]);

    // Central network-mode poll (the banner + AP guard both read the slice).
    useEffect(() => {
        dispatch(fetchConnectionMode());
        const id = setInterval(() => dispatch(fetchConnectionMode()), 10000);
        return () => clearInterval(id);
    }, [dispatch]);

    // AP fallback: without internet the OTP login cannot complete, so an
    // unauthenticated user lands on the offline WiFi-setup page. The /auth
    // routes are exempt — the password login works offline.
    useEffect(() => {
        if (loaded && mode === 'ap' && !isAuthenticated
            && location.pathname !== getSetupUrl()
            && !location.pathname.startsWith(getAuthenticationUrl())) {
            navigate(getSetupUrl(), { replace: true });
        }
    }, [loaded, mode, isAuthenticated, location.pathname, navigate]);

    return (
        <Outlet />
    )
}

export default RouterContext;