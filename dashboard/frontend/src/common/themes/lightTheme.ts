import { createTheme, responsiveFontSizes } from '@mui/material/styles';
import { commonThemeOptions } from './commonThemeOptions';

const lightTheme = createTheme({
    ...commonThemeOptions,
    palette: {
        mode: 'light',
        primary: {
            main: '#8b5cf6',
        },
        secondary: {
            main: '#F092DD',
        },
        text: {
            primary: '#2c1212',
            secondary: '#2c1212',
        },
        background: {
            default: '#F2F2F2',
        },
        accent: {
            main: "#EDBF59"
        }
    },
});

const light = responsiveFontSizes(lightTheme);

export default light;
