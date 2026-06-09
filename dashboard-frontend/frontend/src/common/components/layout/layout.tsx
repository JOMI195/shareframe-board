import { Box, Container, useMediaQuery, useTheme } from "@mui/material";
import TopAppbar from "./topLayout/topAppBar";
import { Outlet } from "react-router";
import { useAppDispatch, useAppSelector } from "@/store";
import { useTimer } from "@/hooks/useTimer";
import { useEffect } from "react";
import { startContinuousStatusCheck } from "@/store/slideshowStatus/slideshowStatus.Slice";
import { fetchDisplayImagesLoopInterval } from "@/store/slideshowOperation/slideshowOperation.Slice";
import { fetchFrameInfos } from "@/store/frameInfo/frameInfo.Slice";
import { fetchLatestRelease } from "@/store/updates/updates.Slice";
import Sidebar from "./sidebar/sidebar";
import PowerDialogs from "@/common/components/powerDialogs";
import { getAuthenticationUrl, getSignOutUrl } from "@/assets/endpoints/app/authEndpoints";
import { getCurrentPath } from "@/store/navigation/navigation.Slice";

const MainLayout = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down("sm"));

    const prevPath = useAppSelector(getCurrentPath);

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

    useEffect(() => {
        if (prevPath !== (getAuthenticationUrl() + getSignOutUrl())) {
            resetAppIntitialLoadTimer();
            startAppIntitialLoadTimer();
        }

        dispatch(fetchDisplayImagesLoopInterval());
        dispatch(fetchFrameInfos());
        dispatch(fetchLatestRelease());
    }, [dispatch]);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: '100vh' }}>
            <TopAppbar />
            <Sidebar />
            <PowerDialogs />
            <Container
                maxWidth="md"
                disableGutters
                sx={{
                    flex: '1 1 auto',
                    display: 'flex',
                    alignItems: "center",
                    flexDirection: 'column',
                    px: isSmallScreen ? 2 : 0,
                    py: isSmallScreen ? 2 : 5,
                }}
            >
                <Outlet />
            </Container>
        </Box>
    );
};

export default MainLayout;
