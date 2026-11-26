import React, { useState } from 'react';
import { Box, Button, CircularProgress, Stack, Typography } from '@mui/material';
import CameraAltIcon from '@mui/icons-material/CameraAlt';
import PhotoLibraryIcon from '@mui/icons-material/PhotoLibrary';
import AddPhotoAlternateOutlinedIcon from '@mui/icons-material/AddPhotoAlternateOutlined';
import { getAllowedExtensionLabels, MAX_INPUT_SIZE_MB } from './imageValidation';
import { PickerSource } from './usePhotoPicker';

interface StartScreenProps {
    openPicker: (source: PickerSource) => void;
    selectFiles: (files: File[]) => void;
    isPreparing: boolean;
    showCamera: boolean;
    maxFiles: number;
}

const StartScreen: React.FC<StartScreenProps> = ({ openPicker, selectFiles, isPreparing, showCamera, maxFiles }) => {
    const [isOver, setIsOver] = useState(false);

    const handleDrop = (event: React.DragEvent<HTMLDivElement>) => {
        event.preventDefault();
        setIsOver(false);
        if (event.dataTransfer.files) {
            selectFiles(Array.from(event.dataTransfer.files));
        }
    };

    return (
        <Box
            onDragOver={(event) => {
                event.preventDefault();
                setIsOver(true);
            }}
            onDragLeave={(event) => {
                event.preventDefault();
                setIsOver(false);
            }}
            onDrop={handleDrop}
            sx={{
                flex: 1,
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                textAlign: 'center',
                gap: 3,
                p: 3,
                m: { xs: 0, md: 1 },
                border: { xs: 'none', md: '2px dashed' },
                borderColor: { md: isOver ? 'primary.main' : 'divider' },
                borderRadius: 2,
                bgcolor: isOver ? 'action.hover' : 'transparent',
                transition: 'border-color 0.2s, background-color 0.2s',
            }}
        >
            <AddPhotoAlternateOutlinedIcon sx={{ fontSize: 72, color: 'text.secondary' }} />
            <Box>
                <Typography variant="h6">Welche Fotos möchtest du hochladen?</Typography>
                <Typography variant="body2" sx={{ color: 'text.secondary', display: { xs: 'none', md: 'block' } }}>
                    Wähle Fotos aus oder ziehe sie hierher.
                </Typography>
            </Box>
            <Stack spacing={1.5} sx={{ width: '100%', maxWidth: 320 }}>
                <Button
                    size="large"
                    variant="contained"
                    disabled={isPreparing}
                    onClick={() => openPicker('gallery')}
                    startIcon={isPreparing ? <CircularProgress size={18} color="inherit" /> : <PhotoLibraryIcon />}
                    sx={{ py: 1.5 }}
                >
                    {isPreparing ? 'Fotos werden vorbereitet…' : 'Fotos auswählen'}
                </Button>
                {showCamera && (
                    <Button
                        size="large"
                        variant="outlined"
                        disabled={isPreparing}
                        onClick={() => openPicker('camera')}
                        startIcon={<CameraAltIcon />}
                        sx={{ py: 1.5 }}
                    >
                        Foto aufnehmen
                    </Button>
                )}
            </Stack>
            <Typography variant="caption" sx={{ color: 'text.secondary' }}>
                {`${getAllowedExtensionLabels().join(', ')} · bis zu ${maxFiles} ${maxFiles === 1 ? 'Foto' : 'Fotos'} auf einmal · je bis ${MAX_INPUT_SIZE_MB} MB`}
            </Typography>
        </Box>
    );
};

export default StartScreen;
