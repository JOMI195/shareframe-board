import { PropsWithChildren, useContext } from 'react';
import { CssBaseline, ThemeProvider, useMediaQuery } from '@mui/material';
import light from '@/common/themes/lightTheme';
import dark from '@/common/themes/darkTheme';
import { ColorThemeContext } from '@/context/colorTheme/colorThemeContextValue';

// Error screens can render above the app's theme provider.
const StandaloneTheme = ({ children }: PropsWithChildren) => {
    const insideApp = useContext(ColorThemeContext) !== undefined;
    const prefersDark = useMediaQuery('(prefers-color-scheme: dark)', { noSsr: true });

    if (insideApp) return children;

    return (
        <ThemeProvider theme={prefersDark ? dark : light}>
            <CssBaseline enableColorScheme />
            {children}
        </ThemeProvider>
    );
};

export default StandaloneTheme;
