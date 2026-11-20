import React, { FormEvent, ReactNode, useId } from 'react';
import { Button, Dialog, DialogContent, Stack, useMediaQuery, useTheme } from '@mui/material';
import AppDialog from '@/common/components/appDialog';
import DialogHeader from '@/common/components/dialogHeader';
import { SlideTransition, ZoomTransition } from '@/common/components/dialogTransitions';

interface FormDialogProps {
    open: boolean;
    title: string;
    submitLabel: string;
    onSubmit: () => void;
    onClose: () => void;
    submitDisabled?: boolean;
    loading?: boolean;
    // A submit that destroys something is red, as in ConfirmDialog.
    destructive?: boolean;
    // Off for dialogs opened while another one closes, where two history entries would race.
    backToClose?: boolean;
    children: ReactNode;
}

// Task dialogs: header with the close X, fullscreen on phones, one submit that Enter also triggers.
const FormDialog: React.FC<FormDialogProps> = ({
    open, title, submitLabel, onSubmit, onClose, submitDisabled = false, loading = false, destructive = false,
    backToClose = true, children,
}) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'), { noSsr: true });
    const titleId = useId();
    const DialogComponent = backToClose ? AppDialog : Dialog;

    const handleSubmit = (event: FormEvent) => {
        event.preventDefault();
        if (!submitDisabled && !loading) onSubmit();
    };

    return (
        <DialogComponent
            open={open}
            onClose={onClose}
            fullScreen={isSmallScreen}
            maxWidth="xs"
            fullWidth
            aria-labelledby={titleId}
            slots={{ transition: isSmallScreen ? SlideTransition : ZoomTransition }}
        >
            <DialogHeader title={title} titleId={titleId} onClose={onClose} />
            <DialogContent>
                <Stack component="form" noValidate onSubmit={handleSubmit} spacing={1} sx={{ pt: 2 }}>
                    {children}
                    <Button type="submit" variant="contained" color={destructive ? 'error' : 'primary'} fullWidth
                        disabled={submitDisabled} loading={loading} sx={{ mt: 1 }}>
                        {submitLabel}
                    </Button>
                </Stack>
            </DialogContent>
        </DialogComponent>
    );
};

export default FormDialog;
