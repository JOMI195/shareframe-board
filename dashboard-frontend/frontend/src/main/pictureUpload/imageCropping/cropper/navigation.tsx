import { Box, Button, Grid, IconButton, Slider, ToggleButton, ToggleButtonGroup, Tooltip, Typography } from '@mui/material';
import React from 'react';

import ZoomInIcon from '@mui/icons-material/ZoomIn';
import ZoomOutIcon from '@mui/icons-material/ZoomOut';
import RotateLeftIcon from '@mui/icons-material/RotateLeft';
import RotateRightIcon from '@mui/icons-material/RotateRight';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowUpwardIcon from '@mui/icons-material/ArrowUpward';
import ArrowDownwardIcon from '@mui/icons-material/ArrowDownward';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import CenterFocusStrongIcon from '@mui/icons-material/CenterFocusStrong';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import FitScreenIcon from '@mui/icons-material/FitScreen';
import { FillColor, stepRotation } from './utils';

const roundZoom = (zoom: number) => Math.round(zoom * 100) / 100;

const ButtonRow: React.FC<{ label: string; children: React.ReactNode }> = ({ label, children }) => (
    <Box
        sx={{
            display: "flex",
            alignItems: "center",
            width: "100%",
            borderRadius: 1,
            bgcolor: (theme) => theme.palette.background.default,
            pl: 2,
            pr: 2,
            py: 0.5,
            mt: 1,
            gap: 1
        }}>
        <Typography variant='body2' sx={{ width: 60, flexShrink: 0 }}>
            {label}
        </Typography>
        {children}
    </Box>
);

interface NavButtonProps {
    label: string;
    onClick: () => void;
    disabled: boolean;
    tooltip?: boolean;
    text?: string;
    children: React.ReactNode;
}

const NavButton: React.FC<NavButtonProps> = ({ label, onClick, disabled, tooltip, text, children }) => {
    const button = (
        <IconButton
            onClick={onClick}
            disabled={disabled}
            aria-label={label}
            sx={{
                flex: '1 1 0',
                minWidth: 0,
                py: 0.25,
                gap: 0.5,
                borderRadius: 1,
                color: 'text.secondary',
                bgcolor: 'background.paper',
                border: 1,
                borderColor: 'divider',
                '&:hover': { bgcolor: 'action.hover' }
            }}
        >
            {children}
            {text && <Typography variant="caption" component="span" sx={{ lineHeight: 1 }}>{text}</Typography>}
        </IconButton>
    );
    if (!tooltip) return button;
    // The wrapper keeps the tooltip working on a disabled button and carries the flex share.
    return (
        <Tooltip title={label} describeChild>
            <Box component="span" sx={{ flex: '1 1 0', minWidth: 0, display: 'flex' }}>{button}</Box>
        </Tooltip>
    );
};

interface RotationControlProps {
    rotation: number;
    onRotation: (rotation: number) => void;
    disabled: boolean;
    ROTATION_DEGREE: number;
    MIN_ROTATION: number;
    MAX_ROTATION: number;
}

const RotationControl: React.FC<RotationControlProps> = ({
    rotation,
    onRotation,
    disabled,
    ROTATION_DEGREE,
    MIN_ROTATION,
    MAX_ROTATION,
}) => {
    const rotate = (direction: 1 | -1) => onRotation(stepRotation(rotation, ROTATION_DEGREE, direction));

    const handleRotationSliderChange = (_: Event, value: number | number[], __: number) => {
        if (typeof value === 'number') {
            onRotation(value);
        }
    };

    return (
        <Box
            sx={{
                display: "flex",
                alignItems: "center",
                width: "100%",
                borderRadius: 1,
                bgcolor: (theme) => theme.palette.background.default,
                pl: 2,
                pr: 1,
                mt: 1
            }}>
            <Typography variant='body2' sx={{
                width: 60
            }}>
                Rotation
            </Typography>
            <IconButton
                onClick={() => rotate(-1)}
                disabled={disabled}
                aria-label="Nach links drehen"
                size="small"
                sx={{ color: 'text.secondary', mr: 1 }}
            >
                <RotateLeftIcon />
            </IconButton>
            <Slider
                value={rotation}
                onChange={handleRotationSliderChange}
                disabled={disabled}
                min={MIN_ROTATION}
                max={MAX_ROTATION}
                step={1}
                aria-label="Drehen"
                getAriaValueText={(value) => `${value} Grad`}
                style={{ flex: 1 }}
                sx={{
                    flex: 1,
                    '& .MuiSlider-thumb': {
                        width: 20,
                        height: 20,
                        '&:hover, &.Mui-focusVisible': {
                            boxShadow: '0px 0px 0px 8px rgba(25, 118, 210, 0.16)',
                        },
                    },
                    '& .MuiSlider-rail': {
                        opacity: 0.28,
                    },
                }}
            />
            <IconButton
                onClick={() => rotate(1)}
                disabled={disabled}
                aria-label="Nach rechts drehen"
                size="small"
                sx={{ color: 'text.secondary', ml: 1 }}
            >
                <RotateRightIcon />
            </IconButton>
        </Box>
    );
};

interface NavigationProps {
    zoom: number;
    onZoom: (zoom: number) => void;
    rotation?: Omit<RotationControlProps, 'disabled'>;
    onMove: (dx: number, dy: number) => void;
    onCenter: () => void;
    onCropperReset: () => void;
    onFitImage?: () => void;
    fitDisabled?: boolean;
    fill?: { color: FillColor; onChange: (color: FillColor) => void };
    disabled: boolean;
    // photo actions get an icon row instead of the full-width buttons
    compact?: boolean;
    ZOOM_FACTOR: number;
    MIN_ZOOM: number;
    MAX_ZOOM: number;
}

export const Navigation: React.FC<NavigationProps> = ({
    onZoom,
    disabled,
    zoom,
    rotation,
    onMove,
    onCenter,
    onCropperReset,
    onFitImage,
    fitDisabled = false,
    fill,
    compact = false,
    ZOOM_FACTOR,
    MIN_ZOOM,
    MAX_ZOOM,
}) => {

    const handleZoomIn = () => {
        if (onZoom && typeof zoom === 'number') {
            const newZoom = Math.min(MAX_ZOOM, roundZoom(zoom + ZOOM_FACTOR));
            onZoom(newZoom);
        }
    };

    const handleZoomOut = () => {
        if (onZoom && typeof zoom === 'number') {
            const newZoom = Math.max(MIN_ZOOM, roundZoom(zoom - ZOOM_FACTOR));
            onZoom(newZoom);
        }
    };

    const handleZoomSliderChange = (_: Event, value: number | number[], __: number) => {
        if (typeof value === 'number') {
            onZoom(value);
        }
    };

    return (
        <Box
            sx={{
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                width: "100%"
            }}>
            <Box
                sx={{
                    display: "flex",
                    alignItems: "center",
                    width: "100%",
                    borderRadius: 1,
                    bgcolor: (theme) => theme.palette.background.default,
                    pl: 2,
                    pr: 1,
                    mt: 1
                }}>
                <Typography variant='body2' sx={{
                    width: 60
                }}>
                    Zoom
                </Typography>
                <IconButton
                    onClick={handleZoomOut}
                    disabled={!!disabled || zoom <= MIN_ZOOM}
                    aria-label="Verkleinern"
                    size="small"
                    sx={{ color: 'text.secondary', mr: 1 }}
                >
                    <ZoomOutIcon />
                </IconButton>
                <Slider
                    value={zoom}
                    onChange={handleZoomSliderChange}
                    disabled={disabled}
                    min={MIN_ZOOM}
                    max={MAX_ZOOM}
                    step={0.01}
                    aria-label="Zoom"
                    getAriaValueText={(value) => `${Math.round(value * 100)} %`}
                    style={{ flex: 1 }}
                    sx={{
                        flex: 1,
                        '& .MuiSlider-thumb': {
                            width: 20,
                            height: 20,
                            '&:hover, &.Mui-focusVisible': {
                                boxShadow: '0px 0px 0px 8px rgba(25, 118, 210, 0.16)',
                            },
                        },
                        '& .MuiSlider-rail': {
                            opacity: 0.28,
                        },
                    }}
                />
                <IconButton
                    onClick={handleZoomIn}
                    disabled={!!disabled || zoom >= MAX_ZOOM}
                    aria-label="Vergrößern"
                    size="small"
                    sx={{ color: 'text.secondary', ml: 1 }}
                >
                    <ZoomInIcon />
                </IconButton>
            </Box>
            {rotation && <RotationControl {...rotation} disabled={disabled} />}
            <ButtonRow label="Position">
                <NavButton label="Nach links verschieben" onClick={() => onMove(-1, 0)} disabled={disabled}>
                    <ArrowBackIcon fontSize="small" />
                </NavButton>
                <NavButton label="Nach oben verschieben" onClick={() => onMove(0, -1)} disabled={disabled}>
                    <ArrowUpwardIcon fontSize="small" />
                </NavButton>
                <NavButton label="Nach unten verschieben" onClick={() => onMove(0, 1)} disabled={disabled}>
                    <ArrowDownwardIcon fontSize="small" />
                </NavButton>
                <NavButton label="Nach rechts verschieben" onClick={() => onMove(1, 0)} disabled={disabled}>
                    <ArrowForwardIcon fontSize="small" />
                </NavButton>
            </ButtonRow>
            {compact && (
                <ButtonRow label="Bild">
                    {onFitImage && (
                        <NavButton label="Volles Bild anzeigen" onClick={onFitImage} disabled={disabled || fitDisabled} tooltip text="Ganz">
                            <FitScreenIcon fontSize="small" />
                        </NavButton>
                    )}
                    <NavButton label="Zentrieren" onClick={onCenter} disabled={disabled} tooltip text="Zentriert">
                        <CenterFocusStrongIcon fontSize="small" />
                    </NavButton>
                    <NavButton label="Zurücksetzen" onClick={onCropperReset} disabled={disabled} tooltip text="Reset">
                        <RestartAltIcon fontSize="small" />
                    </NavButton>
                </ButtonRow>
            )}
            {fill && (
                <ButtonRow label="Rand">
                    <ToggleButtonGroup
                        value={fill.color}
                        exclusive
                        size="small"
                        fullWidth
                        disabled={disabled}
                        aria-label="Randfarbe"
                        onChange={(_, color: FillColor | null) => color && fill.onChange(color)}
                        sx={{ bgcolor: 'background.paper', '& .MuiToggleButton-root': { py: 0, height: 26, textTransform: 'none' } }}
                    >
                        <ToggleButton value="black">Schwarz</ToggleButton>
                        <ToggleButton value="white">Weiß</ToggleButton>
                    </ToggleButtonGroup>
                </ButtonRow>
            )}
            {!compact && (
                <Grid
                    container
                    sx={{
                        justifyContent: "space-between",
                        mt: 1
                    }}>
                    <Grid size={5.8}>
                        <Button
                            disabled={disabled}
                            fullWidth
                            startIcon={<CenterFocusStrongIcon />}
                            sx={{
                                bgcolor: (theme) => theme.palette.background.default,
                                color: (theme) => theme.palette.text.primary,
                            }}
                            onClick={onCenter}
                        >
                            Zentrieren
                        </Button>
                    </Grid>
                    <Grid size={5.8}>
                        <Button
                            disabled={disabled}
                            fullWidth
                            startIcon={<RestartAltIcon />}
                            sx={{
                                bgcolor: (theme) => theme.palette.background.default,
                                color: (theme) => theme.palette.text.primary,
                            }}
                            onClick={onCropperReset}
                        >
                            Zurücksetzen
                        </Button>
                    </Grid>
                </Grid>
            )}
        </Box>
    );
};

export default Navigation;