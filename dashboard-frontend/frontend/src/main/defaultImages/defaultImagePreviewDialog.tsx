import React from 'react';
import { Typography } from '@mui/material';
import { DeleteOutlined, DownloadOutlined } from '@mui/icons-material';
import uuid from 'react-uuid';
import ShareframeMainDialog from '@/common/components/shareframeMainDialog';
import ImagePreviewLayout, { MetaItem } from '@/common/components/imagePreviewLayout';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';
import { getDefaultImageFileUrl } from '@/assets/endpoints/api/frame';
import { DialogAction, IDefaultImage } from '@/types';
import { downloadBoardFile } from '@/common/utils/files/download';
import { useAppDispatch } from '@/store';
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice';

interface DefaultImagePreviewDialogProps {
    image: IDefaultImage | null;
    position: number;
    count: number;
    min: number;
    removing: boolean;
    onRemove: () => void;
    onClose: () => void;
}

const DefaultImagePreviewDialog: React.FC<DefaultImagePreviewDialogProps> = ({
    image,
    position,
    count,
    min,
    removing,
    onRemove,
    onClose,
}) => {
    const dispatch = useAppDispatch();
    const atMinimum = count <= min;

    const handleDownload = async () => {
        if (!image) return;
        try {
            await downloadBoardFile(getDefaultImageFileUrl(image.name), `standardbild-${position}.${image.name.split('.').pop()}`);
        } catch {
            dispatch(addAlertSnackbar(uuid(), 'Bild konnte nicht heruntergeladen werden', 'error'));
        }
    };

    const primaryAction: DialogAction = {
        icon: <DeleteOutlined />,
        onClick: onRemove,
        label: 'Entfernen',
        tooltip: atMinimum ? `Mindestens ${min} Standardbilder bleiben` : undefined,
        color: 'error',
        disabled: atMinimum || removing,
    };

    const dialogActions: DialogAction[] = [
        {
            icon: <DownloadOutlined />,
            onClick: handleDownload,
            label: 'Bild herunterladen',
        },
    ];

    return (
        <ShareframeMainDialog
            open={image !== null}
            onDialogClose={onClose}
            dialogTitle="Standardbild"
            dialogContentSx={{ padding: { xs: 2, md: 3 } }}
            actionsShown={image !== null}
            actionPrimary={primaryAction}
            actionsAdditional={dialogActions}
        >
            {image && (
                <ImagePreviewLayout url={getDefaultImageFileUrl(image.name)} alt={`Standardbild ${position}`} width={image.width} height={image.height}>
                    <MetaItem label="Wiedergabe">
                        <Typography>{`Position ${position} von ${count}`}</Typography>
                    </MetaItem>
                    <MetaItem label="Auflösung">
                        <Typography>{image.width > 0 ? `${image.width} × ${image.height} px` : '–'}</Typography>
                    </MetaItem>
                    <MetaItem label="Dateigröße">
                        <Typography>{image.sizeBytes > 0 ? getReadablyFileSize(image.sizeBytes) : '–'}</Typography>
                    </MetaItem>
                </ImagePreviewLayout>
            )}
        </ShareframeMainDialog>
    );
};

export default DefaultImagePreviewDialog;
