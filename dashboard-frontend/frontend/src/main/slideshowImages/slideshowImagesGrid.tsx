import React from 'react';
import {
    Box,
    Chip,
    IconButton,
    ImageList,
    ImageListItem,
    ImageListItemBar,
    Stack,
    Typography,
    useMediaQuery,
    useTheme,
} from '@mui/material';
import { ChevronLeft, ChevronRight, VisibilityOutlined } from '@mui/icons-material';
import {
    DndContext,
    DragEndEvent,
    KeyboardSensor,
    PointerSensor,
    TouchSensor,
    closestCenter,
    useSensor,
    useSensors,
} from '@dnd-kit/core';
import { SortableContext, arrayMove, rectSortingStrategy, sortableKeyboardCoordinates, useSortable } from '@dnd-kit/sortable';
import { CSS } from '@dnd-kit/utilities';
import AuthenticatedImage from '@/common/components/authenticatedImage';
import UserAvatar from '@/common/components/userAvatar';
import { getImageFileUrl } from '@/assets/endpoints/api/frame';
import { ISlideshowImage } from '@/types';
import { formatUnix } from './slideshowImagesUtils';

const PANEL_ASPECT_RATIO = '5 / 3';

interface SlideshowImagesGridProps {
    images: ISlideshowImage[];
    currentImageId: number | null;
    sortable: boolean;
    onReorder: (ids: number[]) => void;
    onOpen: (id: number) => void;
}

interface TileProps {
    image: ISlideshowImage;
    index: number;
    count: number;
    isCurrent: boolean;
    sortable: boolean;
    onMove: (from: number, to: number) => void;
    onOpen: (id: number) => void;
}

const SlideshowImageTile: React.FC<TileProps> = ({ image, index, count, isCurrent, sortable, onMove, onOpen }) => {
    const theme = useTheme();
    const { attributes, listeners, setNodeRef, transform, transition, isDragging } = useSortable({
        id: image.id,
        disabled: !sortable,
    });

    return (
        <ImageListItem
            ref={setNodeRef}
            {...(sortable ? { ...attributes, ...listeners } : {})}
            aria-label={`Bild von ${image.sender}, Position ${index + 1}`}
            onClick={sortable ? undefined : () => onOpen(image.id)}
            sx={{
                cursor: sortable ? 'grab' : 'pointer',
                aspectRatio: PANEL_ASPECT_RATIO,
                borderRadius: 2,
                overflow: 'hidden',
                outline: sortable ? `2px dashed ${theme.palette.primary.main}` : 'none',
                transform: CSS.Transform.toString(transform),
                transition,
                zIndex: isDragging ? 2 : 'auto',
                opacity: isDragging ? 0.8 : 1,
                touchAction: sortable ? 'none' : 'auto',
            }}
        >
            <AuthenticatedImage
                url={getImageFileUrl(image.id)}
                alt={`Bild von ${image.sender}`}
                style={{ width: '100%', height: '100%', objectFit: 'cover', borderRadius: 8 }}
            />
            <Stack direction="row" spacing={1} sx={{ position: 'absolute', top: 8, left: 8, right: 8, justifyContent: 'space-between' }}>
                <Chip label={index + 1} size="small" sx={{ bgcolor: 'background.paper', fontWeight: 600 }} />
                {isCurrent && (
                    <Chip icon={<VisibilityOutlined />} label="Wird angezeigt" size="small" color="success" />
                )}
            </Stack>
            <ImageListItemBar
                sx={{ borderBottomLeftRadius: 8, borderBottomRightRadius: 8 }}
                title={
                    <Stack direction="row" spacing={1} sx={{ alignItems: 'center' }}>
                        <UserAvatar username={image.sender} size={24} />
                        <Typography variant="body2" noWrap>{image.sender}</Typography>
                    </Stack>
                }
                subtitle={formatUnix(image.receivedAt)}
                actionIcon={sortable ? (
                    <Box sx={{ display: 'flex' }}>
                        <IconButton
                            size="small"
                            sx={{ color: 'common.white' }}
                            aria-label={`Bild von ${image.sender} nach vorne verschieben`}
                            disabled={index === 0}
                            onPointerDown={(e) => e.stopPropagation()}
                            onKeyDown={(e) => e.stopPropagation()}
                            onClick={() => onMove(index, index - 1)}
                        >
                            <ChevronLeft />
                        </IconButton>
                        <IconButton
                            size="small"
                            sx={{ color: 'common.white' }}
                            aria-label={`Bild von ${image.sender} nach hinten verschieben`}
                            disabled={index === count - 1}
                            onPointerDown={(e) => e.stopPropagation()}
                            onKeyDown={(e) => e.stopPropagation()}
                            onClick={() => onMove(index, index + 1)}
                        >
                            <ChevronRight />
                        </IconButton>
                    </Box>
                ) : undefined}
            />
        </ImageListItem>
    );
};

const SlideshowImagesGrid: React.FC<SlideshowImagesGridProps> = ({ images, currentImageId, sortable, onReorder, onOpen }) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));
    const cols = isSmallScreen ? 1 : 2;

    const sensors = useSensors(
        useSensor(PointerSensor, { activationConstraint: { distance: 5 } }),
        useSensor(TouchSensor, { activationConstraint: { delay: 200, tolerance: 5 } }),
        useSensor(KeyboardSensor, { coordinateGetter: sortableKeyboardCoordinates }),
    );

    const ids = images.map((img) => img.id);
    const move = (from: number, to: number) => onReorder(arrayMove(ids, from, to));

    const handleDragEnd = ({ active, over }: DragEndEvent) => {
        if (!over || active.id === over.id) return;
        move(ids.indexOf(Number(active.id)), ids.indexOf(Number(over.id)));
    };

    return (
        <DndContext sensors={sensors} collisionDetection={closestCenter} onDragEnd={handleDragEnd}>
            <SortableContext items={ids} strategy={rectSortingStrategy}>
                <ImageList cols={cols} gap={12} sx={{ overflow: 'visible', m: 0 }}>
                    {images.map((image, index) => (
                        <SlideshowImageTile
                            key={image.id}
                            image={image}
                            index={index}
                            count={images.length}
                            isCurrent={image.id === currentImageId}
                            sortable={sortable}
                            onMove={move}
                            onOpen={onOpen}
                        />
                    ))}
                </ImageList>
            </SortableContext>
        </DndContext>
    );
};

export default SlideshowImagesGrid;
