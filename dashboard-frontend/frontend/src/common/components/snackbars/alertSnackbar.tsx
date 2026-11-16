import React, { useState } from "react";
import { Alert, Slide } from "@mui/material";
import { StackedSnackbar } from "./snackbarStack";
import { AlertSnackbarItem } from "@/store/snackbars/snackbars.Slice";

interface IAlertSnackbarProps {
    snackbar: AlertSnackbarItem;
    removeSnackbar: () => void;
}

const AlertSnackbar = ({ snackbar, removeSnackbar }: IAlertSnackbarProps) => {
    const [open, setOpen] = useState(true);

    const handleClose = (event?: React.SyntheticEvent | Event, reason?: string) => {
        // Buttons that open a snackbar sit outside it too; they opt out of clickaway.
        if (reason === 'clickaway') {
            const target = (event?.target ?? null) as HTMLElement | null;
            if (target?.closest('.ignore-clickaway')) return;
        }
        setOpen(false);
    };

    return (
        <StackedSnackbar
            open={open}
            // Errors stay until dismissed, so there is time to read them.
            autoHideDuration={snackbar.autoHideDuration ?? (snackbar.severity === 'error' ? null : 10000)}
            onClose={handleClose}
            slots={{ transition: Slide }}
            slotProps={{ transition: { onExited: removeSnackbar } }}
        >
            <Alert
                elevation={6}
                variant="standard"
                severity={snackbar.severity}
                onClose={handleClose}
                slotProps={{ closeButton: { 'aria-label': 'Schließen', title: 'Schließen' } }}
                sx={{ width: '100%' }}
            >
                {snackbar.message}
            </Alert>
        </StackedSnackbar>
    );
};

export default AlertSnackbar;
