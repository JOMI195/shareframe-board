import React from 'react';
import { Box, Grid, ListItemAvatar, ListItemText, Stack, Typography, useMediaQuery, useTheme } from '@mui/material';
import { VisibilityOutlined } from '@mui/icons-material';
import ShareframeMainDialog from '@/common/components/shareframeMainDialog';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import UserAvatar from '@/common/components/userAvatar';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';
import { getImageFileUrl } from '@/assets/endpoints/api/frame';
import { ISlideshowImage } from '@/types';
import { formatTimeLeft, formatUnix } from './slideshowImagesUtils';

interface SlideshowImagePreviewDialogProps {
    image: ISlideshowImage | null;
    count: number;
    isCurrent: boolean;
    onClose: () => void;
}

const MetaItem: React.FC<{ label: string; children: React.ReactNode }> = ({ label, children }) => (
    <Grid size={{ xs: 6, sm: 4, md: 2 }}>
        <Box sx={{ display: 'flex', flexDirection: 'column' }}>
            <Typography sx={{ mb: 0.5 }} color="GrayText">{label}</Typography>
            {children}
        </Box>
    </Grid>
);

const SlideshowImagePreviewDialog: React.FC<SlideshowImagePreviewDialogProps> = ({ image, count, isCurrent, onClose }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <ShareframeMainDialog
            open={image !== null}
            onDialogClose={onClose}
            dialogTitle="Bild auf dem Bilderrahmen"
            dialogContentSx={{ padding: 2, mb: 0 }}
        >
            {image && (
                <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', width: '100%' }}>
                    <Grid container spacing={2} sx={{ pb: 2, width: '100%' }}>
                        <MetaItem label="Absender">
                            <Stack direction="row" spacing={-1.5} sx={{ alignItems: 'center' }}>
                                <ListItemAvatar>
                                    <UserAvatar username={image.sender} size={36} />
                                </ListItemAvatar>
                                <ListItemText primary={image.sender} />
                            </Stack>
                        </MetaItem>
                        <MetaItem label="Empfangen am">
                            <Typography>{formatUnix(image.receivedAt)}</Typography>
                        </MetaItem>
                        <MetaItem label="Läuft ab am">
                            <Typography>{formatUnix(image.expiresAt)}</Typography>
                            <Typography variant="caption" color="text.secondary">{formatTimeLeft(image.expiresAt)}</Typography>
                        </MetaItem>
                        <MetaItem label="Auflösung">
                            <Typography>{image.width > 0 ? `${image.width} × ${image.height} px` : '–'}</Typography>
                        </MetaItem>
                        <MetaItem label="Dateigröße">
                            <Typography>{image.sizeBytes > 0 ? getReadablyFileSize(image.sizeBytes) : '–'}</Typography>
                        </MetaItem>
                        <MetaItem label="Wiedergabe">
                            <Typography>{`Position ${image.position} von ${count}`}</Typography>
                            {isCurrent && (
                                <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5, color: 'success.main' }}>
                                    <VisibilityOutlined fontSize="small" />
                                    <Typography variant="caption">Wird gerade angezeigt</Typography>
                                </Box>
                            )}
                        </MetaItem>
                    </Grid>
                    <AuthenticatedImage
                        key={image.id}
                        url={getImageFileUrl(image.id)}
                        alt={`Bild von ${image.sender}`}
                        style={{ maxWidth: '100%', maxHeight: isSmallScreen ? '50vh' : '60vh', borderRadius: 8 }}
                        aspectRatio={image.height ? image.width / image.height : undefined}
                    />
                </Box>
            )}
        </ShareframeMainDialog>
    );
};

export default SlideshowImagePreviewDialog;
