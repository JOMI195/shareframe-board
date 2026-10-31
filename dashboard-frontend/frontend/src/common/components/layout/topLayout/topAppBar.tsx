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
import { useColorThemeContext } from '@/context/colorTheme/colorThemeContextValue';
import { Badge, Typography, useMediaQuery, useTheme } from '@mui/material';
import Logo from '../../logo';
import ConnectionModeChip from './connectionModeChip';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import SignalWifiStatusbar4BarIcon from '@mui/icons-material/SignalWifiStatusbar4Bar';
import InfoIcon from '@mui/icons-material/Info';
import { selectUpdatesState } from '@/store/updates/updates.Slice';
import UpdateIcon from '@mui/icons-material/Update';
import { getBadgeNumber } from '@/common/utils/badge';
import { closeSidebar, getSidebar, openSidedbar } from '@/store/navigation/navigation.Slice';
import { selectAuth } from '@/store/auth/auth.Slice';
import { getAuthenticationUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import { useNavigate } from 'react-router';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';

const TopAppBar = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();

    const { colorMode, toggleColorMode, iconComponent: IconComponent } = useColorThemeContext();
    const { isConnected } = usePiConnection();
    const sidebarOpen = useAppSelector(getSidebar).open;

    const { isAuthenticated } = useAppSelector(selectAuth);

    const { latest_release } = useAppSelector(selectUpdatesState);

    const isNewVersion = latest_release?.is_newer ?? false;

    const theme = useTheme();
    const isPhone = useMediaQuery(theme.breakpoints.down('sm'), { noSsr: true });
    const themeToggleLabel = colorMode === "dark" ? "Wechsel in den hellen Modus" : "Wechsel in den dunklen Modus";

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
                    <IconButton
                        aria-label={`Menü ${sidebarOpen ? "schließen" : "öffnen"}`}
                        aria-expanded={sidebarOpen}
                        onClick={handleSidebarButtonClick}
                        sx={{ mr: 1, width: 44, height: 44 }}
                    >
                        {sidebarOpen ? <MenuOpenIcon /> : <MenuIcon />}
                    </IconButton>
                    <Logo
                        darkLogoSrc="/logo-dark-full-shareframe.svg"
                        lightLogoSrc="/logo-light-full-shareframe.svg"
                        maxWidth={isPhone ? 100 : 130}
                        minWidth={0}
                        marginRight={0}
                        linkUrl={getHomeUrl()}
                        alt="ShareFrame – Start"
                    />
                    <Box
                        sx={{
                            flex: 1
                        }}
                    />
                    <ConnectionModeChip compact={isPhone} />
                    <Tooltip title="Benachrichtigungen">
                        <IconButton
                            onClick={handleInfoClick}
                            aria-label="Benachrichtigungen"
                            aria-haspopup="menu"
                            aria-expanded={open}
                            sx={{ width: 44, height: 44 }}
                        >
                            <Badge badgeContent={badgeNumber} color="error">
                                <InfoIcon />
                            </Badge>
                        </IconButton>
                    </Tooltip>
                    <Tooltip title={themeToggleLabel}>
                        <IconButton onClick={toggleColorMode} aria-label={themeToggleLabel} sx={{ width: 44, height: 44 }}>
                            <IconComponent />
                        </IconButton>
                    </Tooltip>
                    {/* On phones both live in the side menu, the bar has no room for them. */}
                    {isAuthenticated && !isPhone && (
                        <PowerMenu
                            renderTrigger={(openMenu, isOpen) => (
                                <Tooltip title="Energieoptionen">
                                    <IconButton
                                        onClick={openMenu}
                                        aria-label="Energieoptionen"
                                        aria-haspopup="menu"
                                        aria-expanded={isOpen}
                                        sx={{ width: 44, height: 44 }}
                                    >
                                        <PowerSettingsNewIcon />
                                    </IconButton>
                                </Tooltip>
                            )}
                        />
                    )}
                    {isAuthenticated && !isPhone && (
                        <Tooltip title="Abmelden">
                            <IconButton onClick={handleLogoutButtonClick} aria-label="Abmelden" sx={{ width: 44, height: 44 }}>
                                <LogoutIcon />
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
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', whiteSpace: 'normal', maxWidth: 400 }}>
                        <Typography>Keine Benachrichtigungen verfügbar</Typography>
                    </MenuItem>
                )}
                {!isConnected && (
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', whiteSpace: 'normal', maxWidth: 400 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                            <ConnectionStatusIcon color={isConnected ? "success" : "error"} />
                            <Typography>Keine Verbindung zum Bilderrahmen</Typography>
                        </Box>
                    </MenuItem>
                )}
                {isNewVersion && (
                    <MenuItem onClick={handleInfoClose} sx={{ pointerEvents: 'none', whiteSpace: 'normal', maxWidth: 400 }}>
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