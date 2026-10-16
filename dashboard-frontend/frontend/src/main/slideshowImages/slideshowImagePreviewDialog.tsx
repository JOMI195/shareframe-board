import React from 'react';
import { Box, Grid, ListItemAvatar, ListItemText, Stack, Typography, useMediaQuery, useTheme } from '@mui/material';
import { DownloadOutlined, SlideshowOutlined, VisibilityOutlined } from '@mui/icons-material';
import uuid from 'react-uuid';
import ShareframeMainDialog from '@/common/components/shareframeMainDialog';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import UserAvatar from '@/common/components/userAvatar';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';
import { getImageFileUrl } from '@/assets/endpoints/api/frame';
import { DialogAction, ISlideshowImage } from '@/types';
import { fetchWithTimeout } from '@/common/utils/fetch';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectSlideshowStatus } from '@/store/slideshowStatus/slideshowStatus.Slice';
import { selectSlideshowOperation, showImageThunk } from '@/store/slideshowOperation/slideshowOperation.Slice';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { useTimer } from '@/hooks/useTimer';
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
    const dispatch = useAppDispatch();
    const { isActive, isRefreshing } = useAppSelector(selectSlideshowStatus);
    const { isToggling, isClearingDisplay, isSkippingImage, isShowingImage } = useAppSelector(selectSlideshowOperation);
    const { isActive: isActionsTimerActive } = useTimer({ id: 'slideshow-actions-timer' });

    const showBlockedReason =
        isCurrent ? 'Wird bereits angezeigt'
            : !isActive ? 'Diashow pausiert'
                : isRefreshing || isActionsTimerActive ? 'Display wird gerade aktualisiert'
                    : undefined;

    const handleShow = () => {
        if (image) dispatch(showImageThunk(image.id));
    };

    const handleDownload = async () => {
        if (!image) return;
        try {
            const response = await fetchWithTimeout(getImageFileUrl(image.id));
            if (!response.ok) throw new Error(`HTTP ${response.status}`);
            const url = URL.createObjectURL(await response.blob());
            const link = document.createElement('a');
            link.href = url;
            link.download = `${image.sender}-${image.id}.jpg`;
            link.click();
            URL.revokeObjectURL(url);
        } catch {
            dispatch(addAlertSnackbar(uuid(), 'Foto herunterladen fehlgeschlagen', 'error'));
        }
    };

    const primaryAction: DialogAction = {
        icon: <SlideshowOutlined />,
        onClick: handleShow,
        label: 'Jetzt anzeigen',
        tooltip: showBlockedReason,
        color: 'primary',
        disabled: showBlockedReason !== undefined || isToggling || isClearingDisplay || isSkippingImage || isShowingImage,
    };

    const dialogActions: DialogAction[] = [
        {
            icon: <DownloadOutlined />,
            onClick: handleDownload,
            label: 'Foto herunterladen',
        },
    ];

    return (
        <ShareframeMainDialog
            open={image !== null}
            onDialogClose={onClose}
            dialogTitle="Bild auf dem Bilderrahmen"
            dialogContentSx={{ padding: 2 }}
            actionsShown={image !== null}
            actionPrimary={primaryAction}
            actionsAdditional={dialogActions}
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
