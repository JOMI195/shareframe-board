import React, { useState, useRef } from 'react';
import {
    Box,
    SpeedDial,
    SpeedDialAction,
    Fab,
} from "@mui/material";
import MoreHorizIcon from '@mui/icons-material/MoreHoriz';
import { DialogAction } from '@/types';

interface BottomMainFloatingActionsProps {
    actionPrimary?: DialogAction;
    actionSecondary?: DialogAction;
    actionsAdditional?: DialogAction[];
    disabled?: boolean;
    position?: 'fixed' | 'absolute';
    speedDialIcon?: React.ReactNode;
}

const BottomFloatingActions: React.FC<BottomMainFloatingActionsProps> = ({
    actionsAdditional = [],
    actionPrimary,
    actionSecondary,
    disabled = false,
    position = 'fixed',
    speedDialIcon = <MoreHorizIcon />,
}) => {
    const [speedDialOpen, setSpeedDialOpen] = useState(false);
    const actionsRef = useRef<HTMLDivElement>(null);

    const getPositionStyles = () => {
        const baseStyles = {
            display: "flex",
            flexDirection: "row",
            gap: 1,
            zIndex: 1050,
        };

        if (position === 'fixed') {
            return {
                ...baseStyles,
                position: "fixed",
                bottom: { xs: 16, md: 72 },
                right: { xs: 16, md: 72 },
            };
        } else {
            return {
                ...baseStyles,
                position: "absolute",
                bottom: { xs: 16, md: 25 },
                right: { xs: 16, md: 25 },
            };
        }
    };

    return (
        <Box
            ref={actionsRef}
            sx={getPositionStyles()}
        >
            {actionPrimary && (
                <Fab
                    color={actionPrimary.color}
                    size="large"
                    onClick={actionPrimary.onClick}
                    disabled={disabled || actionPrimary.disabled}
                    variant='extended'
                    sx={{ alignSelf: "flex-end", borderRadius: '10px' }}
                >
                    {actionPrimary.icon && (
                        <Box sx={{ mr: { xs: actionPrimary.label ? 1 : 0 }, display: 'flex' }}>
                            {actionPrimary.icon}
                        </Box>
                    )}
                    {actionPrimary.label}
                </Fab>
            )}

            {actionSecondary && (
                <Fab
                    color={actionSecondary.color}
                    size="large"
                    onClick={actionSecondary.onClick}
                    disabled={disabled || actionSecondary.disabled}
                    variant='extended'
                    sx={{ alignSelf: "flex-end", borderRadius: '10px' }}
                >
                    {actionSecondary && (
                        <Box sx={{ mr: { xs: actionSecondary.label ? 1 : 0 }, display: 'flex' }}>
                            {actionSecondary.icon}
                        </Box>
                    )}
                    {actionSecondary.label}
                </Fab>
            )}

            {actionsAdditional.length > 0 && (
                <SpeedDial
                    ariaLabel={"Aktionen"}
                    icon={speedDialIcon}
                    onClose={() => setSpeedDialOpen(false)}
                    onOpen={() => setSpeedDialOpen(true)}
                    open={speedDialOpen}
                    FabProps={{
                        size: "medium",
                        disabled: disabled
                    }}
                    sx={{
                        '& .MuiSpeedDialAction-staticTooltipLabel': {
                            whiteSpace: 'nowrap',
                            minWidth: 'auto',
                            width: 'auto',
                        },
                        '& .MuiTooltip-tooltip': {
                            whiteSpace: 'nowrap',
                        },
                        '& .MuiFab-primary': {
                            borderRadius: '10px',
                        },
                        '& .MuiSpeedDialAction-fab': {
                            borderRadius: '10px',
                        },
                    }}
                >
                    {actionsAdditional.map((action, index) => (
                        <SpeedDialAction
                            key={`${action.label}-${index}`}
                            icon={action.icon}
                            tooltipTitle={action.label}
                            tooltipOpen
                            onClick={() => {
                                setSpeedDialOpen(false);
                                action.onClick();
                            }}
                            FabProps={{
                                disabled: disabled || action.disabled,
                            }}
                            sx={{
                                '& .MuiFab-root': {
                                    backgroundColor: action.color ? `${action.color}.main` : 'default',
                                    '&:hover': {
                                        backgroundColor: action.color ? `${action.color}.light` : 'action.hover'
                                    }
                                },
                                '& .MuiSpeedDialAction-staticTooltipLabel': {
                                    whiteSpace: 'nowrap',
                                    minWidth: 'auto',
                                    maxWidth: 'none',
                                },
                            }}
                        />
                    ))}
                </SpeedDial>
            )}
        </Box>
    );
};

export default BottomFloatingActions;