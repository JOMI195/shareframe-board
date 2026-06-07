import { useEffect } from "react";
import { Paper, Typography, Box, IconButton } from "@mui/material";
import CloseIcon from '@mui/icons-material/Close';
import { AlertSnackbarItem } from "@/store/snackbars/snackbars.Slice";

interface IAlertSnackbarProps {
    snackbar: AlertSnackbarItem;
    closeSnackbar: () => void;
}

const AlertSnackbar = ({ snackbar, closeSnackbar }: IAlertSnackbarProps) => {
    const getBackgroundColor = (severity: string) => {
        switch (severity) {
            case "error":
                return "red";
            case "warning":
                return "orange";
            case "info":
                return "blue";
            case "success":
                return "green";
            default:
                return "grey";
        }
    };

    // Auto-close snackbar after the autoHideDuration (default to 6000ms if not provided)
    useEffect(() => {
        if (!snackbar) return;

        const timer = setTimeout(() => {
            closeSnackbar();
        }, snackbar.autoHideDuration || 10000);

        return () => {
            clearTimeout(timer);
        };
    }, []);


    return (
        <Box
            sx={{
                display: 'flex',
                justifyContent: 'center',
                width: '100%',
                position: 'relative', // Allow stacking based on parent position
            }}
        >
            <Paper
                elevation={5}
                sx={{
                    py: 1.5,
                    px: 3,
                    display: 'flex',
                    alignItems: "center",
                    justifyContent: "space-between",
                    flexDirection: "row",
                    backgroundColor: getBackgroundColor(snackbar.severity), // Color based on severity
                    color: 'white',
                    minWidth: '300px', // Ensuring a minimum width
                    maxWidth: '500px', // Optional: limit the max width
                }}
            >
                <Typography variant="subtitle2" sx={{ ml: 2 }}>
                    {snackbar.message}
                </Typography>
                <IconButton
                    size="small"
                    color="inherit"
                    onClick={closeSnackbar}
                    aria-label="close"
                    sx={{ ml: 2 }}
                >
                    <CloseIcon />
                </IconButton>
            </Paper>
        </Box>
    );
};

export default AlertSnackbar;