import React from 'react';
import { Box } from '@mui/material';
import { useTheme } from '@mui/material/styles';
import { passepartoutBackground, passepartoutShade } from './passepartout';

const PassepartoutOverlay: React.FC = () => {
    const theme = useTheme();

    return (
        <Box
            aria-hidden
            sx={{
                position: 'absolute',
                inset: 0,
                pointerEvents: 'none',
                background: passepartoutBackground(passepartoutShade(theme.palette.mode)),
            }}
        />
    );
};

export default PassepartoutOverlay;
