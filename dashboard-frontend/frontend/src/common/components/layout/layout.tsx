import { Box, Container, Link, useMediaQuery, useTheme } from "@mui/material";
import TopAppbar from "./topLayout/topAppBar";
import NetworkStatusBanner from "@/common/components/networkStatusBanner";
import { Outlet, useLocation } from "react-router";
import { useAppDispatch, useAppSelector } from "@/store";
import { useTimer } from "@/hooks/useTimer";
import { useEffect, useEffectEvent, useRef } from "react";
import { startContinuousStatusCheck } from "@/store/slideshowStatus/slideshowStatus.Slice";
import { fetchDisplayImagesLoopInterval, fetchNightMode } from "@/store/slideshowOperation/slideshowOperation.Slice";
import { fetchFrameInfos } from "@/store/frameInfo/frameInfo.Slice";
import { fetchLatestRelease } from "@/store/updates/updates.Slice";
import Sidebar from "./sidebar/sidebar";
import PowerDialogs from "@/common/components/powerDialogs";
import { getAuthenticationUrl, getSignOutUrl } from "@/assets/endpoints/app/authEndpoints";
import { getCurrentPath, getSidebar, routeChanged } from "@/store/navigation/navigation.Slice";
import HiddenPageTitle from "@/common/components/hiddenPageTitle";
import { usePageTitle } from "@/hooks/navigation/usePageTitle";
import { SIDEBAR_ICON_WIDTH, SIDEBAR_WIDTH } from "./sidebar/sidebarWidth";

const MainLayout = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down("sm"));
    const isDesktop = useMediaQuery(theme.breakpoints.up("lg"), { noSsr: true });
    const sidebarOpen = useAppSelector(getSidebar).open;
    const mainRef = useRef<HTMLElement>(null);
    const pageTitle = usePageTitle();

    const prevPath = useAppSelector(getCurrentPath);
    const { pathname } = useLocation();
    const focusedPathname = useRef(pathname);

    // Screen readers otherwise stay on the link that was activated. Waits for the phone menu,
    // which hands focus back to its button when it closes.
    useEffect(() => {
        if (focusedPathname.current === pathname || (sidebarOpen && !isDesktop)) return;
        focusedPathname.current = pathname;
        mainRef.current?.focus({ preventScroll: true });
    }, [pathname, sidebarOpen, isDesktop]);

    // On leaving a route, so it runs before the next page's mount effects can open a dialog.
    useEffect(() => () => {
        dispatch(routeChanged());
    }, [pathname, dispatch]);

    const { reset: resetAppIntitialLoadTimer, start: startAppIntitialLoadTimer } = useTimer({
        id: 'app-initial-load-timer',
        duration: 20,
    });

    useEffect(() => {
        const stopStatusCheck = dispatch(startContinuousStatusCheck());

        return () => {
            stopStatusCheck();
        };
    }, [dispatch]);

    const restartInitialLoadTimer = useEffectEvent(() => {
        if (prevPath !== (getAuthenticationUrl() + getSignOutUrl())) {
            resetAppIntitialLoadTimer();
            startAppIntitialLoadTimer();
        }
    });

    useEffect(() => {
        restartInitialLoadTimer();

        dispatch(fetchDisplayImagesLoopInterval());
        dispatch(fetchNightMode());
        dispatch(fetchFrameInfos());
        dispatch(fetchLatestRelease());
    }, [dispatch]);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: '100vh' }}>
            <Link
                href="#main"
                sx={{
                    position: 'absolute',
                    left: '50%',
                    transform: 'translateX(-50%)',
                    top: -48,
                    zIndex: (theme) => theme.zIndex.tooltip,
                    p: 1,
                    bgcolor: 'background.paper',
                    borderRadius: 1,
                    '&:focus': { top: 8 },
                }}
            >
                Zum Inhalt springen
            </Link>
            <TopAppbar />
            <NetworkStatusBanner />
            <Sidebar />
            <PowerDialogs />
            <Box
                sx={{
                    flex: '1 1 auto',
                    display: 'flex',
                    // The desktop sidebar is fixed, so the page has to make room for it itself.
                    pl: isDesktop ? `${sidebarOpen ? SIDEBAR_WIDTH : SIDEBAR_ICON_WIDTH}px` : 0,
                    transition: theme.transitions.create('padding-left', { duration: theme.transitions.duration.standard }),
                }}
            >
                <Container
                    component="main"
                    id="main"
                    ref={mainRef}
                    tabIndex={-1}
                    maxWidth="md"
                    disableGutters
                    sx={{
                        outline: 'none',
                        flex: '1 1 auto',
                        display: 'flex',
                        alignItems: "center",
                        flexDirection: 'column',
                        minWidth: 0,
                        px: isSmallScreen ? 2 : 3,
                        pt: isSmallScreen ? 2 : 5,
                        pb: 5,
                    }}
                >
                    {pageTitle && <HiddenPageTitle>{pageTitle}</HiddenPageTitle>}
                    <Outlet />
                </Container>
            </Box>
        </Box>
    );
};

export default MainLayout;
