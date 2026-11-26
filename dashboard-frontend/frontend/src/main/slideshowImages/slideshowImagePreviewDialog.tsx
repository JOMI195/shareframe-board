import React from 'react';
import { Box, Stack, Typography } from '@mui/material';
import { DownloadOutlined, SlideshowOutlined, VisibilityOutlined } from '@mui/icons-material';
import uuid from 'react-uuid';
import ShareframeMainDialog from '@/common/components/shareframeMainDialog';
import ImagePreviewLayout, { MetaItem } from '@/common/components/imagePreviewLayout';
import UserAvatar from '@/common/components/userAvatar';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';
import { getImageFileUrl } from '@/assets/endpoints/api/frame';
import { DialogAction, ISlideshowImage } from '@/types';
import { downloadBoardFile } from '@/common/utils/files/download';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectSlideshowStatus } from '@/store/slideshowStatus/slideshowStatus.Slice';
import { selectSlideshowOperation, showImageThunk } from '@/store/slideshowOperation/slideshowOperation.Slice';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';
import { usePanelRest } from '@/hooks/usePanelRest';
import { formatExpiresAt, formatTimeLeft, formatUnix } from './slideshowImagesUtils';

interface SlideshowImagePreviewDialogProps {
    image: ISlideshowImage | null;
    count: number;
    isCurrent: boolean;
    onClose: () => void;
}

const SlideshowImagePreviewDialog: React.FC<SlideshowImagePreviewDialogProps> = ({ image, count, isCurrent, onClose }) => {
    const dispatch = useAppDispatch();
    const { isActive, isRefreshing } = useAppSelector(selectSlideshowStatus);
    const { isToggling, isClearingDisplay, isSkippingImage, isShowingImage } = useAppSelector(selectSlideshowOperation);
    const resting = usePanelRest() > 0;

    const showBlockedReason =
        isCurrent ? 'Wird bereits angezeigt'
            : !isActive ? 'Diashow pausiert'
                : isRefreshing ? 'Display wird gerade aktualisiert'
                    : resting ? 'Das Display ruht kurz, um es zu schonen'
                    : undefined;

    const handleShow = () => {
        if (image) dispatch(showImageThunk(image.id));
    };

    const handleDownload = async () => {
        if (!image) return;
        try {
            await downloadBoardFile(getImageFileUrl(image.id), `${image.sender}-${image.id}.jpg`);
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
                <ImagePreviewLayout url={getImageFileUrl(image.id)} alt={`Foto von ${image.sender}`} width={image.width} height={image.height}>
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
                        <Typography>{formatExpiresAt(image.expiresAt)}</Typography>
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
                </ImagePreviewLayout>
            )}
        </ShareframeMainDialog>
    );
};

export default SlideshowImagePreviewDialog;
