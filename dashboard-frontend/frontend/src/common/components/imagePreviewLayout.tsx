import React from 'react';
import { Box, Typography, useMediaQuery, useTheme } from '@mui/material';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import { useDesktopActions } from '@/hooks/layout/useDesktopActions';

const DETAILS_WIDTH = 260;
// Fills the desktop dialog below header and actions row without scrolling.
const MAX_HEIGHT_DESKTOP = 'calc(85vh - 165px)';
const FALLBACK_RATIO = 800 / 480;

export const MetaItem: React.FC<{ label: string; children: React.ReactNode }> = ({ label, children }) => (
    <Box sx={{ display: 'flex', flexDirection: 'column', minWidth: 0 }}>
        <Typography sx={{ mb: 0.5 }} color="text.secondary">{label}</Typography>
        {children}
    </Box>
);

interface ImagePreviewLayoutProps {
    url: string;
    alt: string;
    width: number;
    height: number;
    /** MetaItems, beside the picture on desktop and above it on phones. */
    children: React.ReactNode;
}

/** A stored picture at the largest size the dialog allows, with its details. */
const ImagePreviewLayout: React.FC<ImagePreviewLayoutProps> = ({ url, alt, width, height, children }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const isDesktop = useDesktopActions();
    const maxHeight = isDesktop ? MAX_HEIGHT_DESKTOP : isSmallScreen ? '50vh' : '60vh';
    const ratio = width && height ? width / height : FALLBACK_RATIO;

    return (
        <Box
            sx={isDesktop
                ? { display: 'grid', gridTemplateColumns: `minmax(0, 1fr) ${DETAILS_WIDTH}px`, columnGap: 3, alignItems: 'start', width: '100%' }
                : { display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2, width: '100%' }}
        >
            <Box sx={{ display: 'flex', justifyContent: 'center', width: '100%', order: isDesktop ? 0 : 1 }}>
                <Box sx={{ width: `min(100%, calc(${maxHeight} * ${ratio}))`, aspectRatio: `${ratio}` }}>
                    <AuthenticatedImage
                        key={url}
                        url={url}
                        alt={alt}
                        style={{ display: 'block', width: '100%', height: '100%', objectFit: 'contain', borderRadius: 8 }}
                    />
                </Box>
            </Box>
            <Box
                component="aside"
                sx={isDesktop
                    ? { display: 'flex', flexDirection: 'column', gap: 2 }
                    : { display: 'grid', gridTemplateColumns: { xs: 'repeat(2, minmax(0, 1fr))', sm: 'repeat(3, minmax(0, 1fr))' }, gap: 2, alignSelf: 'stretch' }}
            >
                {children}
            </Box>
        </Box>
    );
};

export default ImagePreviewLayout;
