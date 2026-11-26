import React from 'react';
import { Box, ButtonBase, CircularProgress, IconButton } from '@mui/material';
import AddIcon from '@mui/icons-material/Add';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import CloseIcon from '@mui/icons-material/Close';
import ImagePreviewThumb, { PREVIEW_ERROR_MESSAGE } from './imagePreviewThumb';
import { ImageStatus } from './useUploadQueue';

const THUMB_SIZE = 48;

interface ThumbStripProps {
    imageStatuses: ImageStatus[];
    currentImageIndex: number | null;
    imagePreviews: { [id: string]: string };
    previewErrors: { [id: string]: string };
    markPreviewBroken: (id: string, reason: string) => void;
    onSelect: (index: number) => void;
    onRemove: (index: number) => void;
    onAdd: () => void;
    canAdd: boolean;
    isPreparing: boolean;
}

const ThumbStrip: React.FC<ThumbStripProps> = ({
    imageStatuses,
    currentImageIndex,
    imagePreviews,
    previewErrors,
    markPreviewBroken,
    onSelect,
    onRemove,
    onAdd,
    canAdd,
    isPreparing,
}) => (
    <Box
        component="ul"
        aria-label="Ausgewählte Fotos"
        sx={{ display: 'flex', gap: 1, m: 0, px: 2, py: 0.5, overflowX: 'auto', listStyle: 'none', flexShrink: 0 }}
    >
        {imageStatuses.map((imageStatus, index) => {
            const uploaded = imageStatus.status === 'uploaded';
            const current = currentImageIndex === index;
            return (
                <Box component="li" key={imageStatus.id} sx={{ position: 'relative', flexShrink: 0, pt: 0.75, pr: 0.75 }}>
                    <ButtonBase
                        aria-label={imageStatus.file.name}
                        aria-current={current || undefined}
                        disabled={uploaded}
                        onClick={() => onSelect(index)}
                        sx={{
                            position: 'relative',
                            borderRadius: 1,
                            overflow: 'hidden',
                            outline: '3px solid',
                            outlineColor: current ? 'primary.main' : 'transparent',
                            outlineOffset: 1,
                        }}
                    >
                        <Box sx={{ opacity: uploaded ? 0.4 : 1 }}>
                            <ImagePreviewThumb
                                src={imagePreviews[imageStatus.id]}
                                alt=""
                                error={previewErrors[imageStatus.id]}
                                onError={() => markPreviewBroken(imageStatus.id, PREVIEW_ERROR_MESSAGE)}
                                size={THUMB_SIZE}
                            />
                        </Box>
                        {uploaded && (
                            <CheckCircleIcon color="success" sx={{ position: 'absolute', inset: 0, m: 'auto' }} />
                        )}
                    </ButtonBase>
                    {!uploaded && (
                        <IconButton
                            size="small"
                            aria-label={`${imageStatus.file.name} entfernen`}
                            onClick={() => onRemove(index)}
                            sx={{
                                position: 'absolute',
                                top: 0,
                                right: 0,
                                p: 0.25,
                                bgcolor: 'background.paper',
                                boxShadow: 1,
                                '&:hover': { bgcolor: 'background.paper' },
                                // A 44px touch target around the small visible button.
                                '&::after': { content: '""', position: 'absolute', inset: -11 },
                            }}
                        >
                            <CloseIcon sx={{ fontSize: 14 }} />
                        </IconButton>
                    )}
                </Box>
            );
        })}
        {canAdd && (
            <Box component="li" sx={{ flexShrink: 0, pt: 0.75 }}>
                <ButtonBase
                    aria-label="Fotos hinzufügen"
                    disabled={isPreparing}
                    onClick={onAdd}
                    sx={{
                        width: THUMB_SIZE,
                        height: THUMB_SIZE,
                        borderRadius: 1,
                        border: '2px dashed',
                        borderColor: 'divider',
                        color: 'text.secondary',
                    }}
                >
                    {isPreparing ? <CircularProgress size={20} /> : <AddIcon />}
                </ButtonBase>
            </Box>
        )}
    </Box>
);

export default ThumbStrip;
