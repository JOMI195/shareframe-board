import { useEffect, useEffectEvent } from "react";
import { Alert, Slide } from "@mui/material";
import { AlertSnackbarItem } from "@/store/snackbars/snackbars.Slice";

interface IAlertSnackbarProps {
    snackbar: AlertSnackbarItem;
    closeSnackbar: () => void;
}

const AlertSnackbar = ({ snackbar, closeSnackbar }: IAlertSnackbarProps) => {
    // Errors stay until dismissed, so there is time to read them.
    const autoHideDuration = snackbar.autoHideDuration ?? (snackbar.severity === 'error' ? null : 10000);
    const onAutoHide = useEffectEvent(closeSnackbar);

    useEffect(() => {
        if (autoHideDuration === null) return;
        const timer = setTimeout(() => onAutoHide(), autoHideDuration);
        return () => clearTimeout(timer);
    }, [autoHideDuration]);

    return (
        <Slide direction="up" in appear>
            <Alert
                elevation={6}
                variant="standard"
                severity={snackbar.severity}
                onClose={closeSnackbar}
                slotProps={{ closeButton: { 'aria-label': 'Schließen', title: 'Schließen' } }}
                sx={{ width: { xs: '100%', sm: 'auto' }, minWidth: { sm: 300 }, maxWidth: { sm: 560 } }}
            >
                {snackbar.message}
            </Alert>
        </Slide>
    );
};

export default AlertSnackbar;
