import { createContext, useContext } from 'react';
import LightModeIcon from '@mui/icons-material/LightMode';
import DarkModeIcon from '@mui/icons-material/DarkMode';
import light from '@/common/themes/lightTheme';
import dark from '@/common/themes/darkTheme';

export type ColorMode = 'light' | 'dark';
export type Theme = typeof light | typeof dark;
export type IconComponent = typeof LightModeIcon | typeof DarkModeIcon;

export interface ColorThemeContextType {
    theme: Theme;
    toggleColorMode: () => void;
    colorMode: ColorMode;
    iconComponent: IconComponent;
}

export const ColorThemeContext = createContext<ColorThemeContextType | undefined>(undefined);

export const useColorThemeContext = () => {
    const context = useContext(ColorThemeContext);
    if (!context) {
        throw new Error('useColorThemeContext must be used within a ColorThemeProvider');
    }
    return context;
};
