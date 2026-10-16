import React, { useState, useRef } from 'react';
import {
    Box,
    SpeedDial,
    SpeedDialAction,
    Fab,
    Tooltip,
} from "@mui/material";
import MoreHorizIcon from '@mui/icons-material/MoreHoriz';
import { DialogAction } from '@/types';

interface BottomMainFloatingActionsProps {
    actionPrimary?: DialogAction;
    actionSecondary?: DialogAction;
    actionsAdditional?: DialogAction[];
    position?: 'fixed' | 'absolute';
    speedDialIcon?: React.ReactNode;
    speedDialDisabled?: boolean;
    allActionsdisabled?: boolean;
}

const BottomFloatingActions: React.FC<BottomMainFloatingActionsProps> = ({
    actionsAdditional = [],
    actionPrimary,
    actionSecondary,
    position = 'fixed',
    speedDialIcon = <MoreHorizIcon />,
    speedDialDisabled = false,
    allActionsdisabled = false,
}) => {
    const [speedDialOpen, setSpeedDialOpen] = useState(false);
    const actionsRef = useRef<HTMLDivElement>(null);

    const getPositionStyles = () => {
        const baseStyles = {
            display: "flex",
            flexDirection: "row",
            gap: 1,
            zIndex: 1050,
            pointerEvents: 'none',
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
                <Tooltip title={actionPrimary.tooltip}>
                    <span style={{ alignSelf: "flex-end" }}>
                        <Fab
                            color={actionPrimary.color}
                            size="large"
                            onClick={actionPrimary.onClick}
                            disabled={allActionsdisabled || actionPrimary.disabled}
                            variant='extended'
                            sx={{
                                borderRadius: '10px',
                                pointerEvents: 'auto'
                            }}
                        >
                            {actionPrimary.icon && (
                                <Box sx={{ mr: { xs: actionPrimary.label ? 1 : 0 }, display: 'flex' }}>
                                    {actionPrimary.icon}
                                </Box>
                            )}
                            {actionPrimary.label && actionPrimary.label}
                        </Fab>
                    </span>
                </Tooltip>
            )}

            {actionSecondary && (
                <Tooltip title={actionSecondary.tooltip}>
                    <span style={{ alignSelf: "flex-end" }}>
                        <Fab
                            color={actionSecondary.color}
                            size="large"
                            onClick={actionSecondary.onClick}
                            disabled={allActionsdisabled || actionSecondary.disabled}
                            variant='extended'
                            sx={{
                                borderRadius: '10px',
                                pointerEvents: 'auto'
                            }}
                        >
                            {actionSecondary.icon && (
                                <Box sx={{ mr: { xs: actionSecondary.label ? 1 : 0 }, display: 'flex' }}>
                                    {actionSecondary.icon}
                                </Box>
                            )}
                            {actionSecondary.label && actionSecondary.label}
                        </Fab>
                    </span>
                </Tooltip>
            )}

            {actionsAdditional.length > 0 && (
                <SpeedDial
                    ariaLabel={"Aktionen"}
                    icon={speedDialIcon}
                    onClose={() => setSpeedDialOpen(false)}
                    onOpen={() => setSpeedDialOpen(true)}
                    open={speedDialOpen}
                    direction="up"
                    FabProps={{
                        size: "medium",
                        disabled: allActionsdisabled || speedDialDisabled,
                        sx: { pointerEvents: 'auto' }
                    }}
                    sx={{
                        '& .MuiSpeedDial-fab': {
                            pointerEvents: 'auto',
                        },
                        '& .MuiSpeedDial-actions': {
                            pointerEvents: speedDialOpen ? 'auto' : 'none',
                            visibility: speedDialOpen ? 'visible' : 'hidden',
                        },
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
                    {actionsAdditional.map((action, index) => {
                        const isAdditionalActionDisabled = allActionsdisabled || speedDialDisabled || action.disabled;

                        return (
                            <SpeedDialAction
                                key={`${action.label}-${index}`}
                                icon={action.icon}
                                onClick={() => {
                                    if (!isAdditionalActionDisabled) {
                                        setSpeedDialOpen(false);
                                        action.onClick();
                                    }
                                }}
                                slotProps={{
                                    tooltip: { title: action.label, open: true },
                                    fab: { disabled: isAdditionalActionDisabled },
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
                        );
                    })}
                </SpeedDial>
            )}
        </Box>
    );
};

export default BottomFloatingActions;