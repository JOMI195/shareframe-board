import React, { useEffect, useState, PropsWithChildren } from 'react';
import LightModeIcon from '@mui/icons-material/LightMode';
import DarkModeIcon from '@mui/icons-material/DarkMode';
import light from '@/common/themes/lightTheme';
import dark from '@/common/themes/darkTheme';
import { ThemeProvider } from '@mui/material';
import { ColorMode, ColorThemeContext, ColorThemeContextType, IconComponent } from './colorThemeContextValue';

// Provider component
export const ColorThemeProvider: React.FC<PropsWithChildren> = ({ children }) => {
    // Determine initial color mode based on system preference
    const systemPrefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const savedMode = localStorage.getItem('color-mode') as ColorMode | null;
    const initialColorMode: ColorMode = savedMode || (systemPrefersDark ? 'dark' : 'light');

    // State management
    const [colorMode, setColorMode] = useState<ColorMode>(initialColorMode);
    const iconComponent: IconComponent = colorMode === 'dark' ? DarkModeIcon : LightModeIcon;

    // Toggle color mode
    const toggleColorMode = () => {
        const newMode: ColorMode = colorMode === 'light' ? 'dark' : 'light';
        setColorMode(newMode);
        localStorage.setItem('color-mode', newMode);
    };

    // Handle system theme changes
    useEffect(() => {
        const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');

        const handleSystemColorModeChange = (e: MediaQueryListEvent) => {
            const newMode: ColorMode = e.matches ? 'dark' : 'light';
            setColorMode(newMode);
            localStorage.setItem('color-mode', newMode);
        };

        mediaQuery.addEventListener('change', handleSystemColorModeChange);

        return () => {
            mediaQuery.removeEventListener('change', handleSystemColorModeChange);
        };
    }, []);

    const contextValue: ColorThemeContextType = {
        theme: colorMode === 'dark' ? dark : light,
        colorMode,
        toggleColorMode,
        iconComponent,
    };

    return (
        <ColorThemeContext.Provider value={contextValue}>
            <ThemeProvider theme={contextValue.theme}>{children}</ThemeProvider>
        </ColorThemeContext.Provider>
    );
};