import React from 'react';
import { AppBar, Box, IconButton, Toolbar, Typography } from '@mui/material';
import CloseIcon from '@mui/icons-material/Close';

interface DialogHeaderProps {
    title?: string;
    subtitle?: string;
    titleId?: string;
    onClose?: () => void;
    closeDisabled?: boolean;
    start?: React.ReactNode;
    actions?: React.ReactNode;
}

const DialogHeader: React.FC<DialogHeaderProps> = ({ title, subtitle, titleId, onClose, closeDisabled, start, actions }) => (
    <AppBar position="relative" color="inherit" elevation={1}>
        <Toolbar variant="dense" sx={{ gap: 1, minHeight: 56 }}>
            {start}
            <Box sx={{ flex: 1, minWidth: 0 }}>
                {title && (
                    <Typography id={titleId} variant="h6" component="h2" sx={{ lineHeight: 1.2 }}>
                        {title}
                    </Typography>
                )}
                {subtitle && (
                    <Typography variant="caption" component="p" noWrap sx={{ color: 'text.secondary' }}>
                        {subtitle}
                    </Typography>
                )}
            </Box>
            {actions}
            {onClose && (
                <IconButton edge="end" color="inherit" onClick={onClose} disabled={closeDisabled} aria-label="Schließen">
                    <CloseIcon />
                </IconButton>
            )}
        </Toolbar>
    </AppBar>
);

export default DialogHeader;
