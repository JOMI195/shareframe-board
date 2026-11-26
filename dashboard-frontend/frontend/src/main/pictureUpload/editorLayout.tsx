import React from 'react';
import { Box, IconButton } from '@mui/material';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import DialogHeader from '@/common/components/dialogHeader';
import { FRAME_ASPECT_RATIO } from './frameAspectBox';
import { CANVAS_MARGIN } from './frameSize';

interface EditorLayoutProps {
    title: string;
    subtitle?: string;
    onClose: () => void;
    // replaces closing with a step back, e.g. from the preview to the editor
    onBack?: () => void;
    actions?: React.ReactNode;
    strip?: React.ReactNode;
    footer?: React.ReactNode;
    children: React.ReactNode;
}

const EditorLayout: React.FC<EditorLayoutProps> = ({ title, subtitle, onClose, onBack, actions, strip, footer, children }) => (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', minHeight: 0 }}>
        <DialogHeader
            title={title}
            subtitle={subtitle}
            titleId="upload-dialog-title"
            onClose={onBack ? undefined : onClose}
            start={onBack && (
                <IconButton edge="start" color="inherit" onClick={onBack} aria-label="Zurück zum Bearbeiten">
                    <ArrowBackIcon />
                </IconButton>
            )}
            actions={actions}
        />
        {strip}
        <Box sx={{ flex: 1, minHeight: 0, overflowY: 'auto', display: 'flex', flexDirection: 'column', px: { xs: 1, md: 3 } }}>
            {children}
        </Box>
        {footer && (
            <Box sx={{ flexShrink: 0, px: { xs: 2, md: 3 }, pt: 1, borderTop: 1, borderColor: 'divider', pb: 'max(8px, env(safe-area-inset-bottom))' }}>
                {footer}
            </Box>
        )}
    </Box>
);

// Phones stack tabs, canvas and controls; wider screens put tabs and controls beside the canvas.
export const EditorGrid: React.FC<{ children: React.ReactNode }> = ({ children }) => (
    <Box
        sx={{
            flex: { xs: 'none', md: '1 1 0' },
            minHeight: 0,
            display: 'grid',
            gridTemplateColumns: { xs: 'minmax(0, 1fr)', md: 'minmax(0, 1fr) 380px' },
            gridTemplateRows: { xs: 'auto auto auto', md: 'auto minmax(0, 1fr)' },
            gridTemplateAreas: { xs: '"tabs" "canvas" "controls"', md: '"canvas tabs" "canvas controls"' },
            columnGap: 3,
        }}
    >
        {children}
    </Box>
);

export const ToolCanvas: React.FC<{ children?: React.ReactNode }> = ({ children }) => (
    <Box
        sx={{
            gridArea: 'canvas',
            minHeight: 0,
            aspectRatio: { xs: '3 / 2', md: 'auto' },
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            containerType: 'size',
        }}
    >
        {children}
    </Box>
);

export const FramedCanvas: React.FC<{ children: React.ReactNode }> = ({ children }) => (
    <Box
        sx={{
            width: `min(calc(100cqw - ${2 * CANVAS_MARGIN}px), calc((100cqh - ${2 * CANVAS_MARGIN}px) * ${FRAME_ASPECT_RATIO}))`,
        }}
    >
        {children}
    </Box>
);

export const ToolControls: React.FC<{ children?: React.ReactNode }> = ({ children }) => (
    <Box sx={{ gridArea: 'controls', minHeight: 0, overflowY: { md: 'auto' }, pb: 1 }}>
        {children}
    </Box>
);

export default EditorLayout;
