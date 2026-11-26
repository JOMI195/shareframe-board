import React from 'react';
import { Box, Tooltip } from '@mui/material';
import BrokenImageIcon from '@mui/icons-material/BrokenImage';

export const PREVIEW_ERROR_MESSAGE = 'Foto konnte nicht gelesen werden. Bitte erneut auswählen.';

interface ImagePreviewThumbProps {
    src?: string;
    alt: string;
    error?: string;
    onError: () => void;
    size?: number;
}

// Not MUI Avatar: it swallows the img error internally and renders a fallback,
// so there is no way to report why a preview failed.
const ImagePreviewThumb: React.FC<ImagePreviewThumbProps> = ({ src, alt, error, onError, size = 40 }) => {
    const box = { width: size, height: size, flexShrink: 0, display: 'block' };

    if (error || !src) {
        return (
            <Tooltip title={error ?? ''}>
                <Box sx={{
                    ...box,
                    bgcolor: 'action.hover',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                }}>
                    {/* no src yet is the object-URL effect not having run, not a failure */}
                    {error && <BrokenImageIcon fontSize="small" color="error" />}
                </Box>
            </Tooltip>
        );
    }

    return (
        <Box
            component="img"
            src={src}
            alt={alt}
            onError={onError}
            sx={{ ...box, objectFit: 'cover' }}
        />
    );
};

export default ImagePreviewThumb;
