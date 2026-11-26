import React, { useEffect, useLayoutEffect, useRef, useState } from 'react';
import EasyCropper, { Area, MediaSize, Point } from 'react-easy-crop';
import { useTheme } from '@mui/material/styles';
import Navigation from './navigation';
import { ToolCanvas, ToolControls } from '../../editorLayout';
import { frameSize } from '../../frameSize';
import { FRAME_ASPECT_RATIO } from '../../frameAspectBox';
import { passepartoutBackground, passepartoutShade } from '../../passepartout';
import { FillColor, containZoom } from './utils';

const ZOOM_FACTOR = 0.05;
const MOVE_STEP = 5;
const MIN_ZOOM = 0.1;
const MAX_ZOOM = 4;

const ROTATION_DEGREE = 22.5;
const MIN_ROTATION = 0;
const MAX_ROTATION = 360;

interface CropperProps {
    src: string;
    crop: Point;
    setCrop: React.Dispatch<React.SetStateAction<Point>>;
    zoom: number;
    setZoom: React.Dispatch<React.SetStateAction<number>>;
    setCroppedAreaPixels: React.Dispatch<React.SetStateAction<Area>>;
    rotation: number;
    setRotation: React.Dispatch<React.SetStateAction<number>>;
    fillColor: FillColor;
    setFillColor: (color: FillColor) => void;
    onError: () => void;
    children?: React.ReactNode;
}

const Cropper: React.FC<CropperProps> = ({
    src,
    crop,
    setCrop,
    zoom,
    setZoom,
    setCroppedAreaPixels,
    rotation,
    setRotation,
    fillColor,
    setFillColor,
    onError,
    children
}) => {
    const theme = useTheme();
    const cropperRef = useRef<EasyCropper>(null);
    const cropperContainerRef = useRef<HTMLDivElement>(null);
    const [isTouching, setIsTouching] = useState(false);
    const [cropSize, setCropSize] = useState<{ width: number; height: number } | null>(null);
    const [measured, setMeasured] = useState(false);
    const [mediaSize, setMediaSize] = useState<MediaSize | null>(null);

    // The cropper rescales the kept position when its box changes size, so it has to start at the final one.
    useLayoutEffect(() => {
        const container = cropperContainerRef.current;
        if (!container) return;
        const measure = (width: number, height: number) => {
            const size = frameSize(width, height);
            setCropSize(size.width > 0 ? size : null);
        };
        const rect = container.getBoundingClientRect();
        measure(rect.width, rect.height);
        setMeasured(true);
        const observer = new ResizeObserver(([entry]) => measure(entry.contentRect.width, entry.contentRect.height));
        observer.observe(container);
        return () => observer.disconnect();
    }, []);

    const getCropperStyles = () => {
        const isLight = theme.palette.mode === 'light';
        const borderColor = isLight ? 'rgba(0, 0, 0, 0.6)' : 'rgba(255, 255, 255, 0.8)';
        const gridColor = isLight ? 'rgba(0, 0, 0, 0.4)' : 'rgba(255, 255, 255, 0.6)';

        return `
          .reactEasyCrop_CropArea {
            border: 2px solid ${borderColor} !important;
            background: ${passepartoutBackground(passepartoutShade(theme.palette.mode))};
          }
          .reactEasyCrop_CropAreaGrid::before {
            border-left: 1px solid ${gridColor} !important;
            border-right: 1px solid ${gridColor} !important;
          }
          .reactEasyCrop_CropAreaGrid::after {
            border-top: 1px solid ${gridColor} !important;
            border-bottom: 1px solid ${gridColor} !important;
          }
        `;
    };

    const handleTouchStart = () => setIsTouching(true);
    const handleTouchEnd = () => setIsTouching(false);

    useEffect(() => {
        const container = cropperContainerRef.current;
        if (container) {
            container.addEventListener("touchstart", handleTouchStart);
            container.addEventListener("touchend", handleTouchEnd);

            return () => {
                container.removeEventListener("touchstart", handleTouchStart);
                container.removeEventListener("touchend", handleTouchEnd);
            };
        }
    }, []);

    const onCropChange = (newCrop: Point) => setCrop(newCrop);

    const onCropComplete = (_: Area, croppedAreaPixels: Area) => setCroppedAreaPixels(croppedAreaPixels);

    const onZoom = (newZoom: number) => setZoom(newZoom);

    const onRotation = (newZoom: number) => setRotation(newZoom);

    const onMove = (dx: number, dy: number) =>
        setCrop((c) => ({ x: c.x + dx * MOVE_STEP, y: c.y + dy * MOVE_STEP }));

    const onCenter = () => setCrop({ x: 0, y: 0 });

    const onFitImage = () => {
        if (!mediaSize || !cropSize) return;
        setZoom(containZoom(mediaSize, cropSize, rotation, MIN_ZOOM, MAX_ZOOM));
        setCrop({ x: 0, y: 0 });
    };

    const onCropperReset = () => {
        setCrop({ x: 0, y: 0 });
        setZoom(1);
        setRotation(0);
    };

    return (
        <>
            <ToolCanvas>
                <div ref={cropperContainerRef} style={{
                    position: 'relative',
                    alignSelf: 'stretch',
                    width: '100%',
                    backgroundColor: theme.palette.mode === 'light' ? '#e0e0e0' : '#000000',
                }}>
                    <style>{getCropperStyles()}</style>
                    {cropSize && <div aria-hidden style={{
                        position: 'absolute',
                        top: '50%',
                        left: '50%',
                        width: cropSize.width,
                        height: cropSize.height,
                        transform: 'translate(-50%, -50%)',
                        backgroundColor: fillColor,
                    }} />}
                    {measured && <EasyCropper
                        ref={cropperRef}
                        image={src}
                        crop={crop}
                        zoom={zoom}
                        rotation={rotation}
                        aspect={FRAME_ASPECT_RATIO}
                        cropSize={cropSize ?? undefined}
                        onCropChange={onCropChange}
                        onCropComplete={onCropComplete}
                        onCropAreaChange={onCropComplete}
                        onZoomChange={onZoom}
                        onRotationChange={(newRotation) => {
                            if (!isTouching) {
                                onRotation(newRotation);
                            }
                        }}
                        minZoom={MIN_ZOOM}
                        maxZoom={MAX_ZOOM}
                        restrictPosition={false}
                        cropShape="rect"
                        objectFit="horizontal-cover"
                        setMediaSize={setMediaSize}
                        mediaProps={{ onError }}
                    />}
                </div>
            </ToolCanvas>
            <ToolControls>
                <Navigation
                    zoom={zoom}
                    onZoom={onZoom}
                    rotation={{ rotation, onRotation, ROTATION_DEGREE, MIN_ROTATION, MAX_ROTATION }}
                    onMove={onMove}
                    onCenter={onCenter}
                    onCropperReset={onCropperReset}
                    onFitImage={onFitImage}
                    fitDisabled={!mediaSize || !cropSize}
                    fill={{ color: fillColor, onChange: setFillColor }}
                    disabled={false}
                    compact
                    ZOOM_FACTOR={ZOOM_FACTOR}
                    MIN_ZOOM={MIN_ZOOM}
                    MAX_ZOOM={MAX_ZOOM}
                />
                {children}
            </ToolControls>
        </>
    );
};

export default Cropper;
