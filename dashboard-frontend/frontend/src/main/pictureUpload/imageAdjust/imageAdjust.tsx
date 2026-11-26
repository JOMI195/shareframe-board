import React from 'react';
import { Alert, Box, Button, Slider, Typography } from '@mui/material';
import { BlobImage } from '../useBlobImage';
import FrameAspectBox from '../frameAspectBox';
import PassepartoutOverlay from '../passepartoutOverlay';
import { FramedCanvas, ToolCanvas, ToolControls } from '../editorLayout';
import { useLoadedImage } from '../useLoadedImage';
import AdjustPreview from './adjustPreview';
import { ADJUST_RANGE, Adjustment, NEUTRAL_ADJUSTMENT, isNeutral } from './adjust';

const sliders: { key: keyof Adjustment; label: string }[] = [
    { key: 'brightness', label: 'Helligkeit' },
    { key: 'contrast', label: 'Kontrast' },
];

interface ImageAdjustProps {
    epaper: boolean;
    croppedImage: BlobImage;
    fileName: string;
    adjustment: Adjustment;
    setAdjustment: (adjustment: Adjustment) => void;
}

const ImageAdjust: React.FC<ImageAdjustProps> = ({ epaper, croppedImage, fileName, adjustment, setAdjustment }) => {
    const { image, error } = useLoadedImage(croppedImage.url);

    return (
        <>
            <ToolCanvas>
                {error ? (
                    <Alert severity="error">Das zugeschnittene Foto konnte nicht geladen werden.</Alert>
                ) : (
                    <FramedCanvas>
                        <FrameAspectBox>
                            {image && (
                                <>
                                    <AdjustPreview
                                        image={image}
                                        adjustment={adjustment}
                                        epaper={epaper}
                                        alt={`Vorschau von ${fileName}`}
                                    />
                                    <PassepartoutOverlay />
                                </>
                            )}
                        </FrameAspectBox>
                    </FramedCanvas>
                )}
            </ToolCanvas>
            <ToolControls>
                {sliders.map(({ key, label }) => (
                    <Box key={key} sx={{ display: 'flex', alignItems: 'center', gap: 2, px: 1 }}>
                        <Typography variant="body2" sx={{ width: 90, flexShrink: 0 }}>{label}</Typography>
                        <Slider
                            value={adjustment[key]}
                            min={-ADJUST_RANGE}
                            max={ADJUST_RANGE}
                            track={false}
                            marks={[{ value: 0 }]}
                            valueLabelDisplay="auto"
                            onChange={(_event, value) => setAdjustment({ ...adjustment, [key]: value as number })}
                            aria-label={label}
                        />
                    </Box>
                ))}
                <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexWrap: 'wrap', gap: 1, px: 1 }}>
                    <Typography variant="caption" sx={{ color: 'text.secondary' }}>
                        Mehr Kontrast macht das Foto auf dem Schwarz-Weiß-Rahmen klarer.
                    </Typography>
                    <Button
                        size="small"
                        disabled={isNeutral(adjustment)}
                        onClick={() => setAdjustment(NEUTRAL_ADJUSTMENT)}
                    >
                        Zurücksetzen
                    </Button>
                </Box>
            </ToolControls>
        </>
    );
};

export default ImageAdjust;
