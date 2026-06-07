import React from 'react';
import { useColorThemeContext } from '@/context/colorTheme/colorThemeContext';

interface LogoProps {
    darkLogoSrc: string;
    lightLogoSrc: string;
    maxWidth?: number;
    minWidth?: number;
    marginRight?: number;
    clickable?: boolean;
    linkUrl?: string;
}

const Logo: React.FC<LogoProps> = ({
    darkLogoSrc,
    lightLogoSrc,
    maxWidth,
    minWidth,
    marginRight = 16,
}) => {
    const { colorMode } = useColorThemeContext();
    const isDarkMode = colorMode === 'dark';
    const logoSrc = isDarkMode ? darkLogoSrc : lightLogoSrc;

    return (
        <img
            src={logoSrc}
            alt="Logo"
            style={{
                width: '100%',
                maxWidth,
                minWidth,
                marginRight
            }}
        />
    );
};

export default Logo;
