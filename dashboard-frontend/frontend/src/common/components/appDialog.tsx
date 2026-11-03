import React from 'react';
import { Dialog, DialogProps } from '@mui/material';
import { useBackToClose } from '@/hooks/navigation/useBackToClose';

// Every app dialog closes on the back button, like a native sheet.
const AppDialog: React.FC<DialogProps> = (props) => {
    useBackToClose(props.open, () => props.onClose?.({}, 'escapeKeyDown'));
    return <Dialog {...props} />;
};

export default AppDialog;
