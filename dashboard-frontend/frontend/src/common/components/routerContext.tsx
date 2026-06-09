import { useAppDispatch, useAppSelector } from "@/store";
import { setCurrentPath } from "@/store/navigation/navigation.Slice";
import { useEffect } from "react";
import { Outlet, useLocation, useNavigate } from "react-router";
import { fetchConnectionMode, selectConnectionMode } from "@/store/connectionMode/connectionMode.Slice";
import { selectAuth } from "@/store/auth/auth.Slice";
import { getSetupUrl } from "@/assets/endpoints/app/appEndpoints";

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

    // AP fallback: the user cannot complete OTP login without internet, so when
    // the board is hosting its AP and there is no session, send them straight to
    // the offline WiFi-setup page instead of the (unusable) sign-in screen.
    useEffect(() => {
        if (loaded && mode === 'ap' && !isAuthenticated && location.pathname !== getSetupUrl()) {
            navigate(getSetupUrl(), { replace: true });
        }
    }, [loaded, mode, isAuthenticated, location.pathname, navigate]);

    return (
        <Outlet />
    )
}

export default RouterContext;