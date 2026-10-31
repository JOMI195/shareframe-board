import React, { useEffect } from 'react';
import {
    Drawer,
    List,
    ListItem,
    ListItemButton,
    ListItemIcon,
    ListItemText,
    ListSubheader,
    Switch,
    Box,
    SwipeableDrawer,
    useTheme,
    useMediaQuery,
    Tooltip,
} from '@mui/material';
import LogoutIcon from '@mui/icons-material/Logout';
import TuneIcon from '@mui/icons-material/Tune';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import PowerMenu from '@/common/components/powerMenu';
import { useAppDispatch, useAppSelector } from '@/store';
import { closeSidebar, getSidebar, openSidedbar, getAdvancedMode, toggleAdvancedMode } from '@/store/navigation/navigation.Slice';
import { sidebarSections } from '@/assets/sidebarMenu/sideBarMenu';
import { Link as RouterLink, useLocation } from 'react-router';
import { getAuthenticationUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import { IAppBarMenuItem } from '@/types';
import { useBackToClose } from '@/hooks/navigation/useBackToClose';
import { SIDEBAR_ICON_WIDTH, SIDEBAR_WIDTH } from './sidebarWidth';

const toPath = (url: string) => (url.startsWith('/') ? url : `/${url}`);

const Sidebar: React.FC = () => {
    const dispatch = useAppDispatch();
    const open = useAppSelector(getSidebar).open;
    const advancedMode = useAppSelector(getAdvancedMode);
    const theme = useTheme();
    const location = useLocation();

    const isDesktop = useMediaQuery(theme.breakpoints.up('lg'), { noSsr: true });
    const shouldShowIconsOnly = isDesktop && !open;
    const currentWidth = shouldShowIconsOnly ? SIDEBAR_ICON_WIDTH : SIDEBAR_WIDTH;

    useEffect(() => {
        if (isDesktop) dispatch(openSidedbar());
    }, [dispatch, isDesktop]);

    // Below desktop the drawer is a modal: it closes on navigation and on the back button.
    useEffect(() => {
        if (!isDesktop) dispatch(closeSidebar());
    }, [dispatch, isDesktop, location.pathname]);

    useBackToClose(!isDesktop && open, () => dispatch(closeSidebar()));

    // A link to another page closes the drawer through the route change; closing here as well
    // would take its history entry back while the router is still navigating.
    const handleItemClick = (itemPath: string) => {
        if (!isDesktop && itemPath === location.pathname) dispatch(closeSidebar());
    };

    const iconSx = {
        width: shouldShowIconsOnly ? '100%' : 'auto',
        minWidth: shouldShowIconsOnly ? 'auto' : 50,
        justifyContent: 'flex-start',
    };

    const itemSx = {
        borderRadius: 0.5,
        minHeight: 48,
    };

    const withTooltip = (title: string, element: React.ReactElement) => shouldShowIconsOnly ? (
        <Tooltip key={title} title={title} placement="right">{element}</Tooltip>
    ) : element;

    const renderListItem = (item: IAppBarMenuItem) => {
        const itemPath = toPath(item.url);
        const isSignOut = item.url.includes(getSignOutUrl());
        const isActive = !isSignOut && location.pathname === itemPath;

        return withTooltip(item.name, (
            <ListItem key={item.name} disablePadding>
                <ListItemButton
                    component={RouterLink}
                    to={itemPath}
                    onClick={() => handleItemClick(itemPath)}
                    aria-current={isActive ? 'page' : undefined}
                    aria-label={shouldShowIconsOnly ? item.name : undefined}
                    sx={{
                        ...itemSx,
                        backgroundColor: isActive ? 'primary.main' : 'inherit',
                        color: isActive ? 'primary.contrastText' : 'inherit',
                        '&:hover': {
                            backgroundColor: isActive ? 'primary.dark' : 'action.hover',
                        },
                    }}
                >
                    <ListItemIcon sx={{ ...iconSx, color: 'inherit' }}>
                        <item.icon />
                    </ListItemIcon>
                    {!shouldShowIconsOnly && <ListItemText primary={item.name} />}
                </ListItemButton>
            </ListItem>
        ));
    };

    const renderAdvancedToggle = () => withTooltip('Erweiterter Modus', (
        <ListItem disablePadding>
            <ListItemButton
                role="switch"
                aria-checked={advancedMode}
                aria-label="Erweiterter Modus"
                onClick={() => dispatch(toggleAdvancedMode())}
                sx={{ ...itemSx, backgroundColor: advancedMode ? 'action.selected' : 'inherit' }}
            >
                <ListItemIcon sx={iconSx}>
                    <TuneIcon color={advancedMode ? 'primary' : 'inherit'} />
                </ListItemIcon>
                {!shouldShowIconsOnly && <ListItemText primary="Erweitert" />}
                {!shouldShowIconsOnly && (
                    <Switch edge="end" size="small" checked={advancedMode} tabIndex={-1} aria-hidden />
                )}
            </ListItemButton>
        </ListItem>
    ));

    const content = (
        <Box
            sx={{
                width: currentWidth,
                height: '100%',
                p: 1,
                '&:hover': {
                    backgroundColor: shouldShowIconsOnly ? 'action.hover' : 'transparent',
                },
            }}
        >
            <Box
                component="nav"
                aria-label="Hauptmenü"
                sx={{ height: '100%', display: "flex", flexDirection: "column" }}
            >
                <List sx={{ flex: 1 }}>
                    {sidebarSections
                        .filter((sec) => !sec.advanced || advancedMode)
                        .map((sec) => (
                            <React.Fragment key={sec.section ?? 'section'}>
                                {sec.section && !shouldShowIconsOnly && (
                                    <ListSubheader disableSticky sx={{ bgcolor: 'transparent', lineHeight: '2.5em' }}>
                                        {sec.section}
                                    </ListSubheader>
                                )}
                                {sec.items.map((item) => renderListItem(item))}
                            </React.Fragment>
                        ))}
                </List>
                <List>
                    {renderAdvancedToggle()}
                    <PowerMenu
                        anchorOrigin={{ vertical: 'top', horizontal: 'right' }}
                        transformOrigin={{ vertical: 'bottom', horizontal: 'left' }}
                        renderTrigger={(openMenu, isOpen) => withTooltip('Energieoptionen', (
                            <ListItem disablePadding>
                                <ListItemButton
                                    onClick={openMenu}
                                    aria-haspopup="menu"
                                    aria-expanded={isOpen}
                                    aria-label={shouldShowIconsOnly ? 'Energieoptionen' : undefined}
                                    sx={itemSx}
                                >
                                    <ListItemIcon sx={iconSx}>
                                        <PowerSettingsNewIcon />
                                    </ListItemIcon>
                                    {!shouldShowIconsOnly && <ListItemText primary="Energie" />}
                                </ListItemButton>
                            </ListItem>
                        ))}
                    />
                    {renderListItem({ name: 'Abmelden', icon: LogoutIcon, url: getAuthenticationUrl() + getSignOutUrl() })}
                </List>
            </Box>
        </Box>
    );

    const paperSx = {
        top: theme.layout.appbar.height,
        height: `calc(100% - ${theme.layout.appbar.height}px)`,
        width: currentWidth,
        overflowX: 'hidden',
        // Same surface as the desktop sidebar; dark mode would lighten the modal drawer.
        backgroundImage: 'none',
    };

    if (!isDesktop) {
        return (
            <SwipeableDrawer
                open={open}
                onOpen={() => dispatch(openSidedbar())}
                onClose={() => dispatch(closeSidebar())}
                disableSwipeToOpen
                ModalProps={{ keepMounted: true }}
                sx={{ '& .MuiDrawer-paper': paperSx }}
            >
                {content}
            </SwipeableDrawer>
        );
    }

    return (
        <Box
            onClickCapture={(e) => {
                const target = e.target as HTMLElement;
                const isListItemClick = target.closest('.MuiListItemButton-root');
                if (shouldShowIconsOnly && !isListItemClick) dispatch(openSidedbar());
            }}
            sx={{ cursor: shouldShowIconsOnly ? 'pointer' : 'default' }}
        >
            <Drawer
                variant="persistent"
                open
                sx={{
                    width: currentWidth,
                    flexShrink: 0,
                    '& .MuiDrawer-paper': {
                        ...paperSx,
                        transition: theme.transitions.create('width', {
                            easing: theme.transitions.easing.sharp,
                            duration: theme.transitions.duration.standard,
                        }),
                    },
                }}
            >
                {content}
            </Drawer>
        </Box>
    );
};

export default Sidebar;
