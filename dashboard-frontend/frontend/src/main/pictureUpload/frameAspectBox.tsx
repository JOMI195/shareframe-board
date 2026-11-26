import React from 'react';
import { Box, CircularProgress } from '@mui/material';
import { useTheme } from '@mui/material/styles';

export const FRAME_ASPECT_RATIO = 800 / 480;

// Holds the final height while the photo loads, so a step does not grow after it scrolled into view.
const FrameAspectBox: React.FC<{ children?: React.ReactNode }> = ({ children }) => {
    const theme = useTheme();

    return (
        <Box
            sx={{
                position: 'relative',
                width: '100%',
                aspectRatio: FRAME_ASPECT_RATIO,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                bgcolor: theme.palette.mode === 'light' ? '#e0e0e0' : '#000000',
                border: theme.palette.mode === 'light' ? `1px solid ${theme.palette.divider}` : 'none',
            }}
        >
            {children ?? <CircularProgress />}
        </Box>
    );
};

export default FrameAspectBox;
