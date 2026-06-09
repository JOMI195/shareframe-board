import React, { useEffect } from 'react';
import {
    Drawer,
    List,
    ListItem,
    ListItemIcon,
    ListItemText,
    ListSubheader,
    Switch,
    Box,
    ClickAwayListener,
    useTheme,
    useMediaQuery,
    Tooltip,
} from '@mui/material';
import LogoutIcon from '@mui/icons-material/Logout';
import TuneIcon from '@mui/icons-material/Tune';
import { useAppDispatch, useAppSelector } from '@/store';
import { closeSidebar, getSidebar, openSidedbar, getAdvancedMode, toggleAdvancedMode } from '@/store/navigation/navigation.Slice';
import { sidebarSections } from '@/assets/sidebarMenu/sideBarMenu';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';
import { Link as RouterLink, useLocation } from 'react-router';
import { getAuthenticationUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';

const Sidebar: React.FC = () => {
    const dispatch = useAppDispatch();
    const open = useAppSelector(getSidebar).open;
    const advancedMode = useAppSelector(getAdvancedMode);
    const theme = useTheme();
    const location = useLocation();

    const isDesktop = useMediaQuery(theme.breakpoints.up('lg'));
    const fullWidth = 240;
    const iconOnlyWidth = 75;
    const shouldShowIconsOnly = isDesktop && !open;
    const currentWidth = shouldShowIconsOnly ? iconOnlyWidth : fullWidth;

    useEffect(() => {
        if (isDesktop) {
            dispatch(openSidedbar());
        }
    }, []);

    const handleSidebarClose = (event: MouseEvent | TouchEvent | React.MouseEvent) => {
        // Don't close sidebar on desktop when in icon-only mode
        if (isDesktop || shouldShowIconsOnly) {
            return;
        }

        const target = event.target as HTMLElement;
        if (target && target.closest('.ignore-clickaway')) {
            return;
        }
        if (open) {
            dispatch(closeSidebar());
        }
    };

    const sidebarBottomItems = [
        { name: 'Abmelden', icon: LogoutIcon, url: getAuthenticationUrl() + getSignOutUrl() },
    ];

    const renderListItem = (item: any, isBottomItem = false) => {
        const homeUrl = getHomeUrl();
        const itemUrl = !isBottomItem && location.pathname === homeUrl ? item.url : `/${item.url}`;
        const isActive = !isBottomItem && location.pathname === itemUrl;

        const listItemContent = (
            <ListItem
                component={RouterLink}
                to={item.url}
                key={item.name}
                onClick={handleSidebarClose}
                sx={{
                    backgroundColor: isActive ? 'primary.light' : 'inherit',
                    color: isActive ? 'primary.contrastText' : 'inherit',
                    borderRadius: 0.5,
                    //px: shouldShowIconsOnly ? 1 : 2,
                    minHeight: 48,
                    '&:hover': {
                        backgroundColor: isActive ? 'primary.main' : 'action.hover',
                    },
                }}
            >
                <ListItemIcon
                    sx={{
                        width: shouldShowIconsOnly ? '100%' : 'auto',
                        minWidth: shouldShowIconsOnly ? 'auto' : 50,
                        justifyContent: 'flex-start',
                        //mr: shouldShowIconsOnly ? 0 : 2,
                    }}
                >
                    <item.icon />
                </ListItemIcon>
                {!shouldShowIconsOnly && <ListItemText primary={item.name} />}
            </ListItem>
        );

        // Wrap with tooltip when showing icons only
        if (shouldShowIconsOnly) {
            return (
                <Tooltip key={item.name} title={item.name} placement="right">
                    {listItemContent}
                </Tooltip>
            );
        }

        return listItemContent;
    };

    const renderAdvancedToggle = () => {
        const content = (
            <ListItem
                onClick={() => dispatch(toggleAdvancedMode())}
                sx={{
                    backgroundColor: advancedMode ? 'action.selected' : 'inherit',
                    borderRadius: 0.5,
                    minHeight: 48,
                    cursor: 'pointer',
                    '&:hover': { backgroundColor: 'action.hover' },
                }}
            >
                <ListItemIcon
                    sx={{
                        width: shouldShowIconsOnly ? '100%' : 'auto',
                        minWidth: shouldShowIconsOnly ? 'auto' : 50,
                        justifyContent: 'flex-start',
                    }}
                >
                    <TuneIcon color={advancedMode ? 'primary' : 'inherit'} />
                </ListItemIcon>
                {!shouldShowIconsOnly && <ListItemText primary="Erweitert" />}
                {!shouldShowIconsOnly && (
                    <Switch edge="end" size="small" checked={advancedMode} tabIndex={-1} />
                )}
            </ListItem>
        );

        return shouldShowIconsOnly ? (
            <Tooltip title="Erweiterter Modus" placement="right">
                {content}
            </Tooltip>
        ) : content;
    };

    return (
        <ClickAwayListener
            mouseEvent="onMouseUp"
            touchEvent="onTouchStart"
            onClickAway={handleSidebarClose}
        >
            <Box
                onClickCapture={(e) => {
                    // Prevent sidebar from opening if a list item or its child is clicked
                    const target = e.target as HTMLElement;
                    const isListItemClick = target.closest('.MuiListItem-root');

                    if (shouldShowIconsOnly && !open && !isListItemClick) {
                        dispatch(openSidedbar());
                    }
                }}
                sx={{
                    cursor: shouldShowIconsOnly ? 'pointer' : 'default',
                }}
            >
                <Drawer
                    variant="persistent"
                    open={open || shouldShowIconsOnly}
                    sx={{
                        width: currentWidth,
                        flexShrink: 0,
                        '& .MuiDrawer-paper': {
                            top: theme => theme.layout.appbar.height,
                            height: `calc(100% - ${theme.layout.appbar.height}px)`,
                            width: currentWidth,
                            transition: theme.transitions.create('width', {
                                easing: theme.transitions.easing.sharp,
                                duration: theme.transitions.duration.standard,
                            }),
                            overflowX: 'hidden',

                        },
                    }}
                >
                    <Box
                        sx={{
                            width: currentWidth,
                            height: '100%',
                            backgroundColor: 'background.paper',
                            p: 1,
                            borderRadius: 1,
                            '&:hover': {
                                backgroundColor: !open ? 'action.hover' : "none",
                            },
                        }}
                        role="sidebar"
                    >
                        <Box
                            sx={{
                                height: '100%',
                                display: "flex",
                                flexDirection: "column",
                            }}
                        >
                            <List
                                sx={{
                                    flex: 1
                                }}
                            >
                                {sidebarSections
                                    .filter((sec) => !sec.advanced || advancedMode)
                                    .map((sec) => (
                                        <React.Fragment key={sec.section ?? 'section'}>
                                            {sec.section && !shouldShowIconsOnly && (
                                                <ListSubheader
                                                    disableSticky
                                                    sx={{ bgcolor: 'transparent', lineHeight: '2.5em' }}
                                                >
                                                    {sec.section}
                                                </ListSubheader>
                                            )}
                                            {sec.items.map((item) => renderListItem(item))}
                                        </React.Fragment>
                                    ))}
                            </List>
                            <List>
                                {renderAdvancedToggle()}
                                {sidebarBottomItems.map((item) => renderListItem(item, true))}
                            </List>
                        </Box>
                    </Box>
                </Drawer>
            </Box>
        </ClickAwayListener>
    );
};

export default Sidebar;