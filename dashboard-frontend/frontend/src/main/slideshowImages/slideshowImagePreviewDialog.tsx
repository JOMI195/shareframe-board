import React from 'react';
import { Box, Stack, Typography, useMediaQuery, useTheme } from '@mui/material';
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
import { useDesktopActions } from '@/hooks/layout/useDesktopActions';
import { formatTimeLeft, formatUnix } from './slideshowImagesUtils';

interface SlideshowImagePreviewDialogProps {
    image: ISlideshowImage | null;
    count: number;
    isCurrent: boolean;
    onClose: () => void;
}

const DETAILS_WIDTH = 260;
// Fills the desktop dialog below header and actions row without scrolling.
const MAX_HEIGHT_DESKTOP = 'calc(85vh - 165px)';
const FALLBACK_RATIO = 800 / 480;

const MetaItem: React.FC<{ label: string; children: React.ReactNode }> = ({ label, children }) => (
    <Box sx={{ display: 'flex', flexDirection: 'column', minWidth: 0 }}>
        <Typography sx={{ mb: 0.5 }} color="text.secondary">{label}</Typography>
        {children}
    </Box>
);

const SlideshowImagePreviewDialog: React.FC<SlideshowImagePreviewDialogProps> = ({ image, count, isCurrent, onClose }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const isDesktop = useDesktopActions();
    const maxHeight = isDesktop ? MAX_HEIGHT_DESKTOP : isSmallScreen ? '50vh' : '60vh';
    const dispatch = useAppDispatch();
    const { isActive, isRefreshing } = useAppSelector(selectSlideshowStatus);
    const { isToggling, isClearingDisplay, isSkippingImage, isShowingImage } = useAppSelector(selectSlideshowOperation);
    const { isActive: isActionsTimerActive } = useTimer({ id: 'slideshow-actions-timer' });

    const showBlockedReason =
        isCurrent ? 'Wird bereits angezeigt'
            : !isActive ? 'Diashow pausiert'
                : isRefreshing || isActionsTimerActive ? 'Display wird gerade aktualisiert'
                    : undefined;

    const ratio = image?.width && image.height ? image.width / image.height : FALLBACK_RATIO;

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
            dispatch(addAlertSnackbar(uuid(), 'Foto konnte nicht heruntergeladen werden', 'error'));
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
            dialogTitle="Foto auf dem Bilderrahmen"
            dialogContentSx={{ padding: { xs: 2, md: 3 } }}
            actionsShown={image !== null}
            actionPrimary={primaryAction}
            actionsAdditional={dialogActions}
        >
            {image && (
                <Box
                    sx={isDesktop
                        ? { display: 'grid', gridTemplateColumns: `minmax(0, 1fr) ${DETAILS_WIDTH}px`, columnGap: 3, alignItems: 'start', width: '100%' }
                        : { display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2, width: '100%' }}
                >
                    <Box sx={{ display: 'flex', justifyContent: 'center', width: '100%', order: isDesktop ? 0 : 1 }}>
                        <Box sx={{ width: `min(100%, calc(${maxHeight} * ${ratio}))`, aspectRatio: `${ratio}` }}>
                            <AuthenticatedImage
                                key={image.id}
                                url={getImageFileUrl(image.id)}
                                alt={`Foto von ${image.sender}`}
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
                        <MetaItem label="Absender">
                            <Stack direction="row" sx={{ alignItems: 'center', gap: 1, minWidth: 0 }}>
                                <UserAvatar username={image.sender} size={36} />
                                <Typography noWrap>{image.sender}</Typography>
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
                    </Box>
                </Box>
            )}
        </ShareframeMainDialog>
    );
};

export default SlideshowImagePreviewDialog;
