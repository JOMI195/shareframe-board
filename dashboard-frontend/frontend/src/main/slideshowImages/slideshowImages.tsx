import React, { useEffect, useState } from 'react';
import { Alert, Button, Grid, ImageList, ImageListItem, Skeleton, Stack, Typography } from '@mui/material';
import { CloseOutlined, EditOutlined, SaveOutlined } from '@mui/icons-material';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchSlideshowImages, selectSlideshowImages, updatePlaybackOrder } from '@/store/slideshowImages/slideshowImages.Slice';
import { useMinimumLoading } from '@/hooks/loading/useMinimumLoading';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import BottomFloatingActions from '@/common/components/bottomFloatingActions';
import DataNotFound from '@/common/components/dataNotFound';
import { PlaybackMode } from '@/types';
import PlaybackOrderSelect from './playbackOrderSelect';
import SlideshowImagesGrid from './slideshowImagesGrid';
import SlideshowImagePreviewDialog from './slideshowImagePreviewDialog';

const POLL_MS = 15000;

const SlideshowImages: React.FC = () => {
    const dispatch = useAppDispatch();
    const { images, mode, currentImageId, isLoading, isUpdatingOrder, error, lastFetchedAt } = useAppSelector(selectSlideshowImages);
    const [draft, setDraft] = useState<number[] | null>(null);
    const [previewId, setPreviewId] = useState<number | null>(null);
    const isEditing = draft !== null;
    const showSkeleton = useMinimumLoading(isLoading && lastFetchedAt === null);

    useEffect(() => {
        if (isEditing) return;
        dispatch(fetchSlideshowImages());
        const id = setInterval(() => dispatch(fetchSlideshowImages()), POLL_MS);
        return () => clearInterval(id);
    }, [dispatch, isEditing]);

    const byId = new Map(images.map((img) => [img.id, img]));
    const shown = draft ? draft.flatMap((id) => byId.get(id) ?? []) : images;
    const preview = previewId !== null ? byId.get(previewId) ?? null : null;

    const handleModeChange = (next: PlaybackMode) => {
        if (next === 'custom') {
            setDraft(images.map((img) => img.id));
            return;
        }
        setDraft(null);
        dispatch(updatePlaybackOrder({ mode: next }));
    };

    const handleSave = async () => {
        if (!draft) return;
        const result = await dispatch(updatePlaybackOrder({ mode: 'custom', customOrder: draft }));
        if (updatePlaybackOrder.fulfilled.match(result)) setDraft(null);
    };

    const renderSkeleton = () => (
        <ImageList cols={2} gap={12} sx={{ m: 0 }}>
            {[...Array(4)].map((_, index) => (
                <ImageListItem key={index}>
                    <Skeleton variant="rectangular" width="100%" height={160} sx={{ borderRadius: 2 }} />
                </ImageListItem>
            ))}
        </ImageList>
    );

    return (
        <>
            <Grid container spacing={3} sx={{ pb: isEditing ? 12 : 0 }}>
                <Grid size={12}>
                    <ShareframeInfoCard
                        title="Wiedergabereihenfolge"
                        minHeight="100px"
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <PlaybackOrderSelect
                                                value={isEditing ? 'custom' : mode}
                                                disabled={mode === null || isUpdatingOrder}
                                                onChange={handleModeChange}
                                            />
                                            {isEditing && (
                                                <Typography variant="body2" color="text.secondary">
                                                    Ziehe die Bilder in die gewünschte Reihenfolge und speichere sie anschließend.
                                                    Neue Bilder werden hinten angehängt.
                                                </Typography>
                                            )}
                                            {!isEditing && mode === 'shuffle' && (
                                                <Typography variant="body2" color="text.secondary">
                                                    Jedes Bild kommt einmal pro Durchlauf an die Reihe, danach wird neu gemischt.
                                                    Unten siehst du den aktuellen Durchlauf.
                                                </Typography>
                                            )}
                                            {!isEditing && mode === 'custom' && (
                                                <Button
                                                    variant="outlined"
                                                    startIcon={<EditOutlined />}
                                                    onClick={() => setDraft(images.map((img) => img.id))}
                                                    disabled={images.length < 2}
                                                    sx={{ alignSelf: 'flex-start' }}
                                                >
                                                    Reihenfolge bearbeiten
                                                </Button>
                                            )}
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                    />
                </Grid>

                {error && lastFetchedAt === null && (
                    <Grid size={12}>
                        <Alert severity="error">Die Bilder konnten nicht geladen werden.</Alert>
                    </Grid>
                )}

                <Grid size={12}>
                    {showSkeleton ? renderSkeleton() : shown.length > 0 ? (
                        <Stack spacing={1}>
                            <SlideshowImagesGrid
                                images={shown}
                                currentImageId={currentImageId}
                                sortable={isEditing}
                                onReorder={setDraft}
                                onOpen={setPreviewId}
                            />
                            <Typography variant="subtitle2" color="text.secondary" sx={{ textAlign: 'center', pt: 1 }}>
                                {shown.length} {shown.length === 1 ? 'Bild' : 'Bilder'} auf dem Bilderrahmen
                            </Typography>
                        </Stack>
                    ) : lastFetchedAt !== null ? (
                        <DataNotFound notFoundMessage="Keine Bilder auf dem Bilderrahmen. Es werden die Standardbilder angezeigt." />
                    ) : null}
                </Grid>
            </Grid>

            <SlideshowImagePreviewDialog
                image={preview}
                count={images.length}
                isCurrent={preview?.id === currentImageId}
                onClose={() => setPreviewId(null)}
            />

            {isEditing && (
                <BottomFloatingActions
                    actionPrimary={{
                        icon: <SaveOutlined />,
                        label: 'Reihenfolge speichern',
                        color: 'primary',
                        onClick: handleSave,
                        disabled: isUpdatingOrder,
                    }}
                    actionSecondary={{
                        icon: <CloseOutlined />,
                        label: 'Verwerfen',
                        color: 'inherit',
                        onClick: () => setDraft(null),
                        disabled: isUpdatingOrder,
                    }}
                />
            )}
        </>
    );
};

export default SlideshowImages;
