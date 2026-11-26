import React from 'react';
import { Box, Typography } from '@mui/material';
import { useTheme } from '@mui/material/styles';
import { passepartoutShade } from './passepartout';

const swatchSx = { width: 20, height: 12, flexShrink: 0, borderRadius: 0.25 };

const HintRow: React.FC<{ swatch: React.ReactNode; children: React.ReactNode }> = ({ swatch, children }) => (
    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
        {swatch}
        <Typography variant="caption" sx={{ color: 'text.secondary' }}>
            {children}
        </Typography>
    </Box>
);

const PassepartoutHint: React.FC = () => {
    const theme = useTheme();

    return (
        <Box sx={{ width: '100%', mt: 1, display: 'flex', flexDirection: 'column', gap: 0.5 }}>
            <HintRow
                swatch={
                    <Box
                        aria-hidden
                        sx={{
                            ...swatchSx,
                            bgcolor: passepartoutShade(theme.palette.mode),
                            border: 1,
                            borderColor: 'divider',
                        }}
                    />
                }
            >
                Schattierter Rand: wird wahrscheinlich vom Passepartout des Rahmens verdeckt.
            </HintRow>
        </Box>
    );
};

export default PassepartoutHint;
