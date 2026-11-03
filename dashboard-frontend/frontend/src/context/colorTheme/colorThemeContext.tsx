import React, { useCallback, useEffect, useMemo, useState, PropsWithChildren } from 'react';
import LightModeIcon from '@mui/icons-material/LightMode';
import DarkModeIcon from '@mui/icons-material/DarkMode';
import light from '@/common/themes/lightTheme';
import dark from '@/common/themes/darkTheme';
import { ThemeProvider } from '@mui/material';
import { ColorMode, ColorThemeContext, ColorThemeContextType } from './colorThemeContextValue';

export const ColorThemeProvider: React.FC<PropsWithChildren> = ({ children }) => {
    const [colorMode, setColorMode] = useState<ColorMode>(() => {
        const savedMode = localStorage.getItem('color-mode') as ColorMode | null;
        return savedMode || (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
    });

    const toggleColorMode = useCallback(() => {
        setColorMode((mode) => {
            const newMode: ColorMode = mode === 'light' ? 'dark' : 'light';
            localStorage.setItem('color-mode', newMode);
            return newMode;
        });
    }, []);

    useEffect(() => {
        const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        const handleSystemColorModeChange = (e: MediaQueryListEvent) => {
            const newMode: ColorMode = e.matches ? 'dark' : 'light';
            setColorMode(newMode);
            localStorage.setItem('color-mode', newMode);
        };
        mediaQuery.addEventListener('change', handleSystemColorModeChange);
        return () => mediaQuery.removeEventListener('change', handleSystemColorModeChange);
    }, []);

    const theme = colorMode === 'dark' ? dark : light;

    // The browser's status bar follows the app bar instead of staying white in dark mode.
    useEffect(() => {
        document.querySelector('meta[name="theme-color"]')?.setAttribute('content', theme.palette.background.default);
    }, [theme]);

    const contextValue = useMemo<ColorThemeContextType>(() => ({
        theme,
        colorMode,
        toggleColorMode,
        iconComponent: colorMode === 'dark' ? DarkModeIcon : LightModeIcon,
    }), [theme, colorMode, toggleColorMode]);

    return (
        <ColorThemeContext.Provider value={contextValue}>
            <ThemeProvider theme={theme}>{children}</ThemeProvider>
        </ColorThemeContext.Provider>
    );
};
