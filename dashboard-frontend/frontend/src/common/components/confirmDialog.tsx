import React, { ReactNode } from 'react';
import { Button, CircularProgress, Dialog, DialogActions, DialogContent, DialogContentText, DialogTitle } from '@mui/material';
import AppDialog from '@/common/components/appDialog';

interface ConfirmDialogProps {
    open: boolean;
    title: string;
    children?: ReactNode;
    confirmLabel: string;
    onConfirm: () => void;
    onCancel: () => void;
    cancelLabel?: string;
    destructive?: boolean;
    loading?: boolean;
    confirmDisabled?: boolean;
    // Off for dialogs opened while another one closes, where two history entries would race.
    backToClose?: boolean;
}

// Every small question in the app: low-key cancel on the left, one filled action on the right.
const ConfirmDialog: React.FC<ConfirmDialogProps> = ({
    open,
    title,
    children,
    confirmLabel,
    onConfirm,
    onCancel,
    cancelLabel = 'Abbrechen',
    destructive = false,
    loading = false,
    confirmDisabled = false,
    backToClose = true,
}) => {
    const DialogComponent = backToClose ? AppDialog : Dialog;

    return (
        <DialogComponent open={open} onClose={onCancel} maxWidth="xs" fullWidth>
            <DialogTitle>{title}</DialogTitle>
            {children && (
                <DialogContent>
                    {typeof children === 'string' ? <DialogContentText>{children}</DialogContentText> : children}
                </DialogContent>
            )}
            {/* Labels too long for one row stack instead of breaking; wrap-reverse puts the action on top. */}
            <DialogActions
                disableSpacing
                sx={{ px: 3, pb: 2, gap: 1, flexWrap: 'wrap-reverse', '& .MuiButton-root': { whiteSpace: 'nowrap' } }}
            >
                {/* Focus starts on the safe choice, so Enter never destroys anything.
                    aria-disabled while busy: a disabled button would drop focus to the page. */}
                <Button onClick={loading ? undefined : onCancel} aria-disabled={loading || undefined} autoFocus={destructive}>
                    {cancelLabel}
                </Button>
                <Button
                    onClick={loading ? undefined : onConfirm}
                    aria-disabled={loading || undefined}
                    disabled={confirmDisabled}
                    variant="contained"
                    color={destructive ? 'error' : 'primary'}
                    autoFocus={!destructive}
                    startIcon={loading ? <CircularProgress size={16} color="inherit" aria-label="Wird ausgeführt" /> : undefined}
                >
                    {confirmLabel}
                </Button>
            </DialogActions>
        </DialogComponent>
    );
};

export default ConfirmDialog;
