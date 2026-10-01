import React, { ReactNode } from "react";
import {
    Dialog,
    DialogTitle,
    DialogContent,
    Button,
    AppBar,
    Toolbar,
    IconButton,
    Typography,
    Box,
    useMediaQuery,
    useTheme,
    Grid
} from "@mui/material";
import CloseIcon from '@mui/icons-material/Close';
import { SlideTransition, ZoomTransition } from "./dialogTransitions";

interface ShareframeDialogProps {
    open: boolean;
    title: string;
    onClose: () => void;
    onConfirm?: () => void;
    confirmText?: string;
    cancelText?: string;
    maxWidth?: 'xs' | 'sm' | 'md' | 'lg' | 'xl';
    fullWidth?: boolean;
    showActions?: boolean;
    confirmDisabled?: boolean;
    children: ReactNode;
}

const ShareframeDialog: React.FC<ShareframeDialogProps> = ({
    open,
    title,
    onClose,
    onConfirm,
    confirmText = "Zustimmen",
    cancelText = "Abbrechen",
    maxWidth = "sm",
    fullWidth = true,
    showActions = true,
    confirmDisabled: disableConfirm = false,
    children
}) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <Dialog
            fullScreen={isSmallScreen}
            open={open}
            onClose={onClose}
            maxWidth={maxWidth}
            fullWidth={fullWidth}
            slots={{
                transition: isSmallScreen ? SlideTransition : ZoomTransition
            }}
        >
            {isSmallScreen ? (
                <AppBar sx={{ position: 'relative' }} color='inherit'>
                    <Toolbar>
                        <IconButton
                            edge='start'
                            color='inherit'
                            onClick={onClose}
                            aria-label='close'
                        >
                            <CloseIcon />
                        </IconButton>
                        <Typography sx={{ ml: 2, flex: 1 }} variant='h6' component='div'>
                            {title}
                        </Typography>
                    </Toolbar>
                </AppBar>
            ) : (
                <DialogTitle>{title}</DialogTitle>
            )}

            <DialogContent>
                <Box sx={{ p: 1 }}>
                    {children}
                    {showActions && (
                        <Grid
                            container
                            spacing={1}
                            sx={{
                                display: "flex",
                                justifyContent: "flex-end",
                                alignItems: "center",
                                width: "100%",
                                mt: 2
                            }}>
                            <Grid size={6}>
                                <Button
                                    variant="outlined"
                                    color="secondary"
                                    onClick={onClose}
                                    sx={{ px: 5 }}
                                    fullWidth
                                >
                                    {cancelText}
                                </Button>
                            </Grid>
                            {onConfirm && (
                                <Grid size={6}>
                                    <Button
                                        variant="contained"
                                        onClick={onConfirm}
                                        disabled={disableConfirm}
                                        fullWidth
                                        sx={{ mr: 1, px: 5 }}
                                    >
                                        {confirmText}
                                    </Button>
                                </Grid>
                            )}
                        </Grid>
                    )}
                </Box>
            </DialogContent>
        </Dialog>
    );
};

export default ShareframeDialog;