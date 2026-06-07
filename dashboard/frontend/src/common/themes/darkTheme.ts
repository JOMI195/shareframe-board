import { createTheme, responsiveFontSizes } from '@mui/material/styles';
import { commonThemeOptions } from './commonThemeOptions';

const darkTheme = createTheme({
    ...commonThemeOptions,
    palette: {
        mode: 'dark',
        primary: {
            main: '#8b5cf6',
        },
        secondary: {
            main: '#ff9e9e',
        },
        text: {
            primary: '#fffdfd',
            secondary: '#fffdfd',
        },
        background: {
            default: '#252627',
        },
        accent: {
            main: "#EDBF59"
        }
    },
});

const dark = responsiveFontSizes(darkTheme);

export default dark;
