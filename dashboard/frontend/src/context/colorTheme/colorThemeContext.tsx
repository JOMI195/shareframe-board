import React, { createContext, useContext, useEffect, useState, PropsWithChildren } from 'react';
import LightModeIcon from '@mui/icons-material/LightMode';
import DarkModeIcon from '@mui/icons-material/DarkMode';
import light from '@/common/themes/lightTheme';
import dark from '@/common/themes/darkTheme';
import { ThemeProvider } from '@mui/material';

// Define types
type ColorMode = 'light' | 'dark';
type Theme = typeof light | typeof dark;
type IconComponent = typeof LightModeIcon | typeof DarkModeIcon;

interface ColorThemeContextType {
    theme: Theme;
    toggleColorMode: () => void;
    colorMode: ColorMode;
    iconComponent: IconComponent;
}

// Create the context
const ColorThemeContext = createContext<ColorThemeContextType | undefined>(undefined);

// Custom hook for easier access
export const useColorThemeContext = () => {
    const context = useContext(ColorThemeContext);
    if (!context) {
        throw new Error('useColorThemeContext must be used within a ColorThemeProvider');
    }
    return context;
};

// Provider component
export const ColorThemeProvider: React.FC<PropsWithChildren<{}>> = ({ children }) => {
    // Determine initial color mode based on system preference
    const systemPrefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const savedMode = localStorage.getItem('color-mode') as ColorMode | null;
    const initialColorMode: ColorMode = savedMode || (systemPrefersDark ? 'dark' : 'light');

    // State management
    const [colorMode, setColorMode] = useState<ColorMode>(initialColorMode);
    const [iconComponent, setIconComponent] = useState<IconComponent>(
        initialColorMode === 'dark' ? DarkModeIcon : LightModeIcon
    );

    // Toggle color mode
    const toggleColorMode = () => {
        const newMode: ColorMode = colorMode === 'light' ? 'dark' : 'light';
        setColorMode(newMode);
        localStorage.setItem('color-mode', newMode);
    };

    // Sync icon with color mode
    useEffect(() => {
        setIconComponent(colorMode === 'dark' ? DarkModeIcon : LightModeIcon);
    }, [colorMode]);

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