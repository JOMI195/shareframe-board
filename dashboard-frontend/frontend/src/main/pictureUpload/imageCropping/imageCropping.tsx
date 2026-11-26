import React from 'react';
import { Alert } from '@mui/material';
import { Area, Point } from 'react-easy-crop';
import Cropper from './cropper/cropper';
import PassepartoutHint from '../passepartoutHint';
import { ImageStatus } from '../useUploadQueue';
import { PREVIEW_ERROR_MESSAGE } from '../imagePreviewThumb';
import { ToolCanvas, ToolControls } from '../editorLayout';
import { FillColor } from './cropper/utils';

interface ImageCroppingProps {
    currentImage: ImageStatus;
    preview?: string;
    previewError?: string;
    markPreviewBroken: (id: string, reason: string) => void;
    setCroppedAreaPixels: React.Dispatch<React.SetStateAction<Area>>;
    crop: Point;
    setCrop: React.Dispatch<React.SetStateAction<Point>>;
    zoom: number;
    setZoom: React.Dispatch<React.SetStateAction<number>>;
    rotation: number;
    setRotation: React.Dispatch<React.SetStateAction<number>>;
    fillColor: FillColor;
    setFillColor: (color: FillColor) => void;
}

const ImageCropping: React.FC<ImageCroppingProps> = ({
    currentImage,
    preview,
    previewError,
    markPreviewBroken,
    setCroppedAreaPixels,
    crop,
    setCrop,
    zoom,
    setZoom,
    rotation,
    setRotation,
    fillColor,
    setFillColor,
}) => {
    if (previewError) {
        return (
            <>
                <ToolCanvas>
                    <Alert severity="error" sx={{ width: '100%' }}>
                        {`${currentImage.file.name}: ${previewError}`}
                    </Alert>
                </ToolCanvas>
                <ToolControls />
            </>
        );
    }

    if (!preview) return <ToolCanvas />;

    return (
        <Cropper
            src={preview}
            crop={crop}
            setCrop={setCrop}
            zoom={zoom}
            setZoom={setZoom}
            setCroppedAreaPixels={setCroppedAreaPixels}
            rotation={rotation}
            setRotation={setRotation}
            fillColor={fillColor}
            setFillColor={setFillColor}
            onError={() => markPreviewBroken(currentImage.id, PREVIEW_ERROR_MESSAGE)}
        >
            <PassepartoutHint />
        </Cropper>
    );
};

export default ImageCropping;
