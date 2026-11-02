import React from "react";
import { Button, Stack, SxProps, Theme, Tooltip } from "@mui/material";
import { DialogAction } from "@/types";
import { useDesktopActions } from "@/hooks/layout/useDesktopActions";

interface PageActionsBarProps {
    // Left to right, the last one is the main action.
    actions: DialogAction[];
    // Fills every button, for surfaces where outlines get lost.
    contained?: boolean;
    sx?: SxProps<Theme>;
}

const PageActionsBar: React.FC<PageActionsBarProps> = ({ actions, contained = false, sx }) => {
    const isDesktop = useDesktopActions();
    if (!isDesktop || actions.length === 0) return null;

    return (
        <Stack direction="row" sx={[{ gap: 1, justifyContent: "flex-end", flexWrap: "wrap", alignSelf: "stretch" }, ...(Array.isArray(sx) ? sx : [sx])]}>
            {actions.map((action, index) => {
                const button = (
                    <Button
                        key={action.label ?? action.tooltip ?? index}
                        variant={contained || index === actions.length - 1 ? "contained" : "outlined"}
                        color={action.color}
                        startIcon={action.icon}
                        onClick={action.onClick}
                        disabled={action.disabled}
                    >
                        {action.label ?? action.tooltip}
                    </Button>
                );
                return action.label && action.tooltip ? (
                    <Tooltip key={action.label} title={action.tooltip}>
                        <span>{button}</span>
                    </Tooltip>
                ) : button;
            })}
        </Stack>
    );
};

export default PageActionsBar;
