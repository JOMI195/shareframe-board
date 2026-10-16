import React from 'react';
import {
    Dialog,
    DialogProps,
    Box,
    useMediaQuery,
    useTheme,
    IconButton,
    Typography,
    AppBar,
    Toolbar,
    DialogContent,
    Theme,
    SxProps,
} from "@mui/material";
import CloseIcon from '@mui/icons-material/Close';
import { SlideTransition, ZoomTransition } from "./dialogTransitions";
import { DialogAction } from '@/types';
import BottomFloatingActions from './bottomFloatingActions';

interface ReusableDialogProps extends Omit<DialogProps, 'title'> {
    // main dialog props
    dialogTitle?: string;
    dialogSubtitle?: string;
    dialogContentSx?: SxProps<Theme>;
    onDialogClose: () => void;
    // actions props
    actionsShown?: boolean;
    actionPrimary?: DialogAction;
    actionsAdditional?: DialogAction[];
    actionsDisabled?: boolean;
}

const ShareframeMainDialog: React.FC<ReusableDialogProps> = ({
    dialogTitle,
    dialogSubtitle,
    dialogContentSx,
    onDialogClose,
    actionsShown = false,
    actionPrimary,
    actionsAdditional = [],
    actionsDisabled = false,
    children,
    ...dialogProps
}) => {
    const theme = useTheme();
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <Dialog
            maxWidth="xl"
            fullWidth
            fullScreen={isSmallScreen}
            {...dialogProps}
            onClose={onDialogClose}
            slots={{
                transition: !isSmallScreen ? ZoomTransition : SlideTransition
            }}
            slotProps={{
                paper: {
                    sx: {
                        maxHeight: isSmallScreen ? '100%' : '85vh',
                        display: 'flex',
                        flexDirection: 'column'
                    }
                }
            }}>
            <AppBar sx={{ position: 'relative', py: 1 }} color='inherit'>
                <Toolbar variant="dense">
                    <Box sx={{ flex: 1, alignSelf: "center" }}>
                        {dialogTitle && (
                            <Typography variant='h6' component='div'>
                                {dialogTitle}
                            </Typography>
                        )}
                        {dialogSubtitle && (
                            <Typography variant='subtitle2' component='div'>
                                {dialogSubtitle}
                            </Typography>
                        )}
                    </Box>

                    <IconButton
                        edge='start'
                        color='inherit'
                        onClick={onDialogClose}
                        aria-label='close'
                        sx={{ alignSelf: "flex-start" }}
                    >
                        <CloseIcon />
                    </IconButton>
                </Toolbar>
            </AppBar>

            <DialogContent
                sx={{
                    overflow: 'auto',
                    flexGrow: 1,
                    mb: 10,
                    ...dialogContentSx
                }}
            >
                <Box
                    sx={{
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center"
                    }}>
                    {children}
                </Box>
            </DialogContent>

            {actionsShown && ((actionsAdditional.length > 0) || (actionPrimary)) && (
                <BottomFloatingActions
                    actionPrimary={actionPrimary}
                    actionsAdditional={actionsAdditional}
                    allActionsdisabled={actionsDisabled}
                    position={"absolute"}
                />
            )}
        </Dialog>
    );
};

export default ShareframeMainDialog;