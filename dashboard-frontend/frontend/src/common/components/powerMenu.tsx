import React, { useState } from 'react';
import { Menu, MenuItem, ListItemIcon, ListItemText, PopoverOrigin } from '@mui/material';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import { useAppDispatch } from '@/store';
import { openRestartDialog, openShutdownDialog } from '@/store/dialogs/dialogs.Slice';

interface PowerMenuProps {
    // Caller supplies the trigger (icon button in the top bar, list item in the
    // sidebar). `open` opens the menu anchored to the clicked element.
    renderTrigger: (open: (e: React.MouseEvent<HTMLElement>) => void, isOpen: boolean) => React.ReactNode;
    anchorOrigin?: PopoverOrigin;
    transformOrigin?: PopoverOrigin;
}

// Shared "Energieoptionen" menu. Only dispatches the global confirm-dialog open
// actions; the dialogs themselves are rendered once in the layout (PowerDialogs).
const PowerMenu: React.FC<PowerMenuProps> = ({
    renderTrigger,
    anchorOrigin = { vertical: 'bottom', horizontal: 'right' },
    transformOrigin = { vertical: 'top', horizontal: 'right' },
}) => {
    const dispatch = useAppDispatch();
    const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
    const open = Boolean(anchorEl);

    const handleOpen = (e: React.MouseEvent<HTMLElement>) => setAnchorEl(e.currentTarget);
    const handleClose = () => setAnchorEl(null);

    const handleRestart = () => { handleClose(); dispatch(openRestartDialog()); };
    const handleShutdown = () => { handleClose(); dispatch(openShutdownDialog()); };

    return (
        <>
            {renderTrigger(handleOpen, open)}
            <Menu
                anchorEl={anchorEl}
                open={open}
                onClose={handleClose}
                anchorOrigin={anchorOrigin}
                transformOrigin={transformOrigin}
            >
                <MenuItem onClick={handleRestart}>
                    <ListItemIcon><RestartAltIcon fontSize="small" /></ListItemIcon>
                    <ListItemText>Neustarten</ListItemText>
                </MenuItem>
                <MenuItem onClick={handleShutdown}>
                    <ListItemIcon><PowerSettingsNewIcon fontSize="small" color="error" /></ListItemIcon>
                    <ListItemText>Herunterfahren</ListItemText>
                </MenuItem>
            </Menu>
        </>
    );
};

export default PowerMenu;
