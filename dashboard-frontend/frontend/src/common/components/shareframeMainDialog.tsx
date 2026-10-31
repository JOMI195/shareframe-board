import React, { useId } from 'react';
import AppDialog from '@/common/components/appDialog';
import {
    DialogProps,
    Box,
    useMediaQuery,
    useTheme,
    DialogContent,
    Theme,
    SxProps,
} from "@mui/material";
import { SlideTransition, ZoomTransition } from "./dialogTransitions";
import { DialogAction } from '@/types';
import BottomFloatingActions from './bottomFloatingActions';
import DialogHeader from './dialogHeader';

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
    const titleId = useId();

    return (
        <AppDialog
            aria-labelledby={dialogTitle ? titleId : undefined}
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
            <DialogHeader title={dialogTitle} subtitle={dialogSubtitle} titleId={titleId} onClose={onDialogClose} />

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
        </AppDialog>
    );
};

export default ShareframeMainDialog;