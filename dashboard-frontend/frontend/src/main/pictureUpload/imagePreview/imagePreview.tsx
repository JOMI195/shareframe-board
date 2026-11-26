import React from 'react';
import { Box, Typography } from '@mui/material';
import { BlobImage } from '../useBlobImage';
import FrameAspectBox from '../frameAspectBox';
import PassepartoutOverlay from '../passepartoutOverlay';
import { EditorGrid, FramedCanvas, ToolCanvas } from '../editorLayout';
import { useEpaperPreview } from '@/main/pictureUpload/epaper/useEpaperPreview';

interface ImagePreviewProps {
    epaper: boolean;
    finalImage: BlobImage;
    fileName: string;
}

const ImagePreview: React.FC<ImagePreviewProps> = ({
    epaper,
    finalImage,
    fileName,
}) => {
    const epaperUrl = useEpaperPreview(finalImage.blob, epaper);

    return (
        <EditorGrid>
            {/* takes the place of the tool tabs, so the photo stays where it was while editing */}
            <Box sx={{ gridArea: 'tabs', minHeight: 48, display: 'flex', alignItems: 'center', justifyContent: 'center', borderBottom: 1, borderColor: 'divider' }}>
                <Typography variant="subtitle2" component="p" sx={{ color: 'text.secondary' }}>
                    Vorschau
                </Typography>
            </Box>
            <ToolCanvas>
                <FramedCanvas>
                    <FrameAspectBox>
                        <Box
                            component="img"
                            src={epaperUrl ?? finalImage.url}
                            alt={`Vorschau von ${fileName}`}
                            sx={{ display: 'block', width: '100%', height: '100%', objectFit: 'contain' }}
                        />
                        <PassepartoutOverlay />
                    </FrameAspectBox>
                </FramedCanvas>
            </ToolCanvas>
        </EditorGrid>
    );
};

export default ImagePreview;
