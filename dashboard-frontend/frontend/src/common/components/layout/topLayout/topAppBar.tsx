import AppBar from '@mui/material/AppBar';
import Box from '@mui/material/Box';
import Toolbar from '@mui/material/Toolbar';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import MenuIcon from '@mui/icons-material/Menu';
import MenuOpenIcon from '@mui/icons-material/MenuOpen';
import { useTheme } from '@mui/material';
import { useColorThemeContext } from '@/context/colorTheme/colorThemeContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import { closeSidebar, getSidebar, openSidedbar } from '@/store/navigation/navigation.Slice';
import { getHomeUrl } from '@/assets/endpoints/app/appEndpoints';
import Logo from '../../logo';
import SystemStatusMenu from './systemStatusMenu';

const LOGO_WIDTH = 130;

const TopAppBar = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();

    const { colorMode, toggleColorMode, iconComponent: IconComponent } = useColorThemeContext();
    const sidebarOpen = useAppSelector(getSidebar).open;
    const themeToggleLabel = colorMode === "dark" ? "Wechsel in den hellen Modus" : "Wechsel in den dunklen Modus";

    const handleSidebarButtonClick = () => {
        dispatch(sidebarOpen ? closeSidebar() : openSidedbar());
    }

    return (
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
                <Box sx={{ display: 'flex', flexShrink: 0, width: LOGO_WIDTH }}>
                    <Logo
                        darkLogoSrc="/logo-dark-full-shareframe.svg"
                        lightLogoSrc="/logo-light-full-shareframe.svg"
                        maxWidth={LOGO_WIDTH}
                        marginRight={0}
                        linkUrl={getHomeUrl()}
                        alt="ShareFrame – Start"
                    />
                </Box>
                <Box sx={{ flex: 1 }} />
                <SystemStatusMenu />
                <Tooltip title={themeToggleLabel}>
                    <IconButton onClick={toggleColorMode} aria-label={themeToggleLabel} sx={{ width: 44, height: 44 }}>
                        <IconComponent />
                    </IconButton>
                </Tooltip>
            </Toolbar>
        </AppBar>
    );
}

export default TopAppBar;
