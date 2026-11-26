import React, { useEffect, useState } from 'react';
import {
    Grid,
    ImageList,
    ImageListItem,
    ImageListItemBar,
    Skeleton,
    Stack,
    Typography,
    useMediaQuery,
    useTheme,
} from '@mui/material';
import { AddPhotoAlternateOutlined, Refresh } from '@mui/icons-material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    fetchDefaultImages,
    removeDefaultImage,
    selectDefaultImages,
    uploadDefaultImage,
} from '@/store/defaultImages/defaultImages.Slice';
import { useMinimumLoading } from '@/hooks/loading/useMinimumLoading';
import { useDesktopActions } from '@/hooks/layout/useDesktopActions';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import BottomFloatingActions from '@/common/components/bottomFloatingActions';
import PageActionsBar from '@/common/components/pageActionsBar';
import ConfirmDialog from '@/common/components/confirmDialog';
import DataNotFound from '@/common/components/dataNotFound';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import { getReadablyFileSize } from '@/common/utils/files/fileSize.helpers';
import { getDefaultImageFileUrl } from '@/assets/endpoints/api/frame';
import { DialogAction } from '@/types';
import PictureUploadDialog from '@/main/pictureUpload/uploadDialog';
import DefaultImagePreviewDialog from './defaultImagePreviewDialog';

const PANEL_ASPECT_RATIO = '5 / 3';

const DefaultImages: React.FC = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const isDesktop = useDesktopActions();
    const { images, min, max, isLoading, isChanging, error, lastFetchedAt } = useAppSelector(selectDefaultImages);
    const showSkeleton = useMinimumLoading(isLoading && lastFetchedAt === null);
    const [uploadOpen, setUploadOpen] = useState(false);
    const [previewName, setPreviewName] = useState<string | null>(null);
    const [confirmRemove, setConfirmRemove] = useState(false);

    useEffect(() => {
        dispatch(fetchDefaultImages());
    }, [dispatch]);

    const previewIndex = images.findIndex((image) => image.name === previewName);
    const preview = previewIndex !== -1 ? images[previewIndex] : null;
    const full = images.length >= max;

    const uploadAction: DialogAction = {
        icon: <AddPhotoAlternateOutlined />,
        label: 'Bild hinzufügen',
        color: 'primary',
        onClick: () => setUploadOpen(true),
        disabled: full || isChanging || lastFetchedAt === null,
    };

    const handleUpload = async (file: File) =>
        uploadDefaultImage.fulfilled.match(await dispatch(uploadDefaultImage(file)));

    const handleRemove = async () => {
        if (!preview) return;
        const result = await dispatch(removeDefaultImage(preview.name));
        setConfirmRemove(false);
        if (removeDefaultImage.fulfilled.match(result)) setPreviewName(null);
    };

    const renderSkeleton = () => (
        <ImageList cols={isSmallScreen ? 2 : 3} gap={12} sx={{ m: 0 }}>
            {[...Array(3)].map((_, index) => (
                <ImageListItem key={index}>
                    <Skeleton variant="rectangular" width="100%" sx={{ aspectRatio: PANEL_ASPECT_RATIO, height: 'auto', borderRadius: 2 }} />
                </ImageListItem>
            ))}
        </ImageList>
    );

    return (
        <>
            <PageActionsBar actions={[uploadAction]} sx={{ mb: 3 }} />
            <Grid container spacing={3}>
                <Grid size={12}>
                    <ShareframeInfoCard
                        title="Standardbilder"
                        minHeight="100px"
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={1}>
                                            <Typography variant="body2" color="text.secondary">
                                                Solange keine Fotos geschickt wurden, zeigt der Bilderrahmen abwechselnd diese Bilder.
                                            </Typography>
                                            <Typography variant="body2" color="text.secondary">
                                                {`Es braucht mindestens ${min} und höchstens ${max} Bilder. Sie müssen sich deutlich unterscheiden, denn immer wieder dasselbe Motiv nutzt das E-Paper-Display ab.`}
                                            </Typography>
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                    />
                </Grid>

                <Grid size={12}>
                    {showSkeleton ? renderSkeleton() : images.length > 0 ? (
                        <Stack spacing={1}>
                            <ImageList cols={isSmallScreen ? 2 : 3} gap={12} sx={{ overflow: 'visible', m: 0 }}>
                                {images.map((image, index) => (
                                    <ImageListItem
                                        key={image.name}
                                        aria-label={`Standardbild ${index + 1}`}
                                        onClick={() => setPreviewName(image.name)}
                                        sx={{ cursor: 'pointer', aspectRatio: PANEL_ASPECT_RATIO, borderRadius: 2, overflow: 'hidden' }}
                                    >
                                        <AuthenticatedImage
                                            url={getDefaultImageFileUrl(image.name)}
                                            alt={`Standardbild ${index + 1}`}
                                            style={{ width: '100%', height: '100%', objectFit: 'cover', borderRadius: 8 }}
                                        />
                                        <ImageListItemBar
                                            sx={{ borderBottomLeftRadius: 8, borderBottomRightRadius: 8 }}
                                            title={`Bild ${index + 1}`}
                                            subtitle={image.width > 0 ? `${image.width} × ${image.height} · ${getReadablyFileSize(image.sizeBytes)}` : undefined}
                                        />
                                    </ImageListItem>
                                ))}
                            </ImageList>
                            <Typography variant="subtitle2" color="text.secondary" sx={{ textAlign: 'center', pt: 1 }}>
                                {`${images.length} von höchstens ${max} Standardbildern`}
                            </Typography>
                        </Stack>
                    ) : error ? (
                        <DataNotFound
                            notFoundMessage="Standardbilder konnten nicht geladen werden"
                            action={{ label: 'Erneut versuchen', icon: <Refresh />, onClick: () => dispatch(fetchDefaultImages()) }}
                        />
                    ) : null}
                </Grid>
            </Grid>

            {!isDesktop && <BottomFloatingActions actionPrimary={uploadAction} />}

            <PictureUploadDialog
                open={uploadOpen}
                onClose={() => setUploadOpen(false)}
                onUpload={handleUpload}
                maxFiles={Math.max(1, max - images.length)}
                uploading={isChanging}
                title="Standardbild hinzufügen"
            />

            <DefaultImagePreviewDialog
                image={preview}
                position={previewIndex + 1}
                count={images.length}
                min={min}
                removing={isChanging}
                onRemove={() => setConfirmRemove(true)}
                onClose={() => setPreviewName(null)}
            />

            <ConfirmDialog
                open={confirmRemove && preview !== null}
                title="Standardbild entfernen?"
                confirmLabel="Entfernen"
                cancelLabel="Behalten"
                destructive
                loading={isChanging}
                onConfirm={handleRemove}
                onCancel={() => setConfirmRemove(false)}
            >
                Das Bild wird vom Bilderrahmen gelöscht. Das lässt sich nicht rückgängig machen.
            </ConfirmDialog>
        </>
    );
};

export default DefaultImages;
