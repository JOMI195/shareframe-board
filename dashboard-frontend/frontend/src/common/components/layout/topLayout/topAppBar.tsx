import React, { useState } from 'react';
import AppBar from '@mui/material/AppBar';
import Box from '@mui/material/Box';
import Toolbar from '@mui/material/Toolbar';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import MenuIcon from '@mui/icons-material/Menu';
import MenuOpenIcon from '@mui/icons-material/MenuOpen';
import LogoutIcon from '@mui/icons-material/Logout';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import PowerMenu from '@/common/components/powerMenu';
import { useColorThemeContext } from '@/context/colorTheme/colorThemeContext';
import { Badge, Typography, useMediaQuery, useTheme } from '@mui/material';
import Logo from '../../logo';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useAppDispatch, useAppSelector } from '@/store';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import SignalWifiStatusbar4BarIcon from '@mui/icons-material/SignalWifiStatusbar4Bar';
import InfoIcon from '@mui/icons-material/Info';
import { selectUpdatesState } from '@/store/updates/updates.Slice';
import { selectFrameInfoState } from '@/store/frameInfo/frameInfo.Slice';
import { isVersionNewer } from '@/common/utils/version';
import UpdateIcon from '@mui/icons-material/Update';
import { getBadgeNumber } from '@/common/utils/badge';
import { closeSidebar, getSidebar, openSidedbar } from '@/store/navigation/navigation.Slice';
import { selectAuth } from '@/store/auth/auth.Slice';
import { getAuthenticationUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import { useNavigate } from 'react-router';

const TopAppBar = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();

    const { colorMode, toggleColorMode, iconComponent: IconComponent } = useColorThemeContext();
    const { isConnected } = usePiConnection();
    const sidebarOpen = useAppSelector(getSidebar).open;

    const { isAuthenticated } = useAppSelector(selectAuth);

    const { latest_release } = useAppSelector(selectUpdatesState);
    const { frameInfo } = useAppSelector(selectFrameInfoState);

    const isNewVersion = (latest_release && frameInfo && isVersionNewer(latest_release.version, frameInfo.version)) ?? false;

    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('md'));

    const handleSidebarButtonClick = () => {
        dispatch(sidebarOpen ? closeSidebar() : openSidedbar());
    }

    const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
    const open = Boolean(anchorEl);

    const handleInfoClick = (event: React.MouseEvent<HTMLButtonElement>) => {
        setAnchorEl(event.currentTarget);
    };

    const handleInfoClose = () => {
        setAnchorEl(null);
    };

    const handleLogoutButtonClick = () => {
        navigate(getAuthenticationUrl() + getSignOutUrl());
    }

    const ConnectionStatusIcon = isConnected ? SignalWifiStatusbar4BarIcon : SignalWifiStatusbarConnectedNoInternet4Icon;

    const badgeNumber = getBadgeNumber(!isConnected, isNewVersion);

    return (
        <>
            <AppBar
                elevation={1}
                position="sticky"
                sx={{
                    height: theme => theme.layout.appbar.height,
                    background: theme => theme.palette.background.default,
                    zIndex: theme.zIndex.drawer + 1,
                }}
            >
                <Toolbar sx={{ height: "100%" }}>
                    <Tooltip title={`Seitenmenü ${sidebarOpen ? "schließen" : "öffnen"}`} placement="bottom">
                        <IconButton
                            size="small"
                            aria-label="toggle sidebar"
                            aria-haspopup="true"
                            onClick={handleSidebarButtonClick}
                            className='ignore-clickaway'
                            sx={{ mr: 2 }}
                        >
                            {sidebarOpen ? <MenuOpenIcon /> : <MenuIcon />}
                        </IconButton>
                    </Tooltip>
                    <Logo
                        darkLogoSrc="/logo-dark-full-shareframe.svg"
                        lightLogoSrc="/logo-light-full-shareframe.svg"
                        maxWidth={130}
                        marginRight={0}
                        clickable={false}
                    />
                    <Box
                        sx={{
                            flex: 1
                        }}
                    />
                    <Tooltip title={"Benachrichtigungen"}>
                        <IconButton onClick={handleInfoClick}>
                            <Badge badgeContent={badgeNumber} color="error">
                                <InfoIcon fontSize={isSmallScreen ? "medium" : "medium"} />
                            </Badge>
                        </IconButton>
                    </Tooltip>
                    <Tooltip title={colorMode === "dark" ? "Wechsel in den hellen Modus" : "Wechsel in den dunklen Modus"}>
                        <IconButton size={isSmallScreen ? "medium" : "medium"} onClick={toggleColorMode}>
                            <IconComponent fontSize={isSmallScreen ? "medium" : "medium"} />
                        </IconButton>
                    </Tooltip>
                    {isAuthenticated && (
                        <PowerMenu
                            renderTrigger={(open) => (
                                <Tooltip title={"Energieoptionen"}>
                                    <IconButton onClick={open}>
                                        <PowerSettingsNewIcon fontSize="medium" />
                                    </IconButton>
                                </Tooltip>
                            )}
                        />
                    )}
                    {isAuthenticated && (
                        <Tooltip title={"Abmelden"}>
                            <IconButton onClick={handleLogoutButtonClick}>
                                <LogoutIcon fontSize={isSmallScreen ? "medium" : "medium"} />
                            </IconButton>
                        </Tooltip>
                    )}
                </Toolbar>
            </AppBar>

            {/* Connection Status Menu */}
            <Menu
                anchorEl={anchorEl}
                open={open}
                onClose={handleInfoClose}
                anchorOrigin={{
                    vertical: 'bottom',
                    horizontal: 'right',
                }}
                transformOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                }}
            >
                {badgeNumber === 0 && (
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', width: "100%", minWidth: "400px" }}>
                        <Typography>Keine Benachrichtigungen verfügbar</Typography>
                    </MenuItem>
                )}
                {!isConnected && (
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', width: "100%", minWidth: "400px" }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                            <ConnectionStatusIcon color={isConnected ? "success" : "error"} />
                            <Typography>Keine Verbindung zum Bilderrahmen</Typography>
                        </Box>
                    </MenuItem>
                )}
                {isNewVersion && (
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', width: "100%", minWidth: "400px" }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                            <UpdateIcon color={"error"} />
                            <Typography>{`Neue Version (${latest_release?.version}) verfügbar`}</Typography>
                        </Box>
                    </MenuItem>
                )}
            </Menu>
        </>
    );
}

export default TopAppBar;