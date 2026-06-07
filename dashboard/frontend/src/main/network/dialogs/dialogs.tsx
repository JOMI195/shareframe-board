import React, { useState } from 'react';
import {
    Typography, TextField, IconButton, InputAdornment
} from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import ShareframeDialog from '@/common/components/shareframeDialog';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    closeNetworkAddNetworkDialog,
    closeNetworkForgetNetworkDialog,
    closeNetworkRenameNetworkDialog,
    getDialogs,
    updateNetworkRenameNetworkName
} from '@/store/dialogs/dialogs.Slice';
import {
    addNetwork,
    forgetNetwork,
    renameNetwork,
    NetworkCredentials,
    selectNetworkState
} from '@/store/network/network.Slice';

const Dialogs: React.FC = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { loading } = useAppSelector(selectNetworkState);
    const dialogs = useAppSelector(getDialogs).network;

    const [newNetwork, setNewNetwork] = useState<NetworkCredentials>({ ssid: '', password: '' });
    const [submitting, setSubmitting] = useState<boolean>(false);
    const [showPassword, setShowPassword] = useState(false);

    const isActionsDisabled = submitting || loading || !isConnected;

    const handleSubmitNetwork = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(addNetwork(newNetwork)).unwrap();
            dispatch(closeNetworkAddNetworkDialog());
            setNewNetwork({ ssid: '', password: '' });
        } catch (error) {
            // Error handling is done in the thunk
        } finally {
            setSubmitting(false);
        }
    };

    const handleForgetNetwork = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(forgetNetwork(dialogs.forgetNetwork.ssid)).unwrap();
            dispatch(closeNetworkForgetNetworkDialog());
        } catch (error) {
            // Error handling is done in the thunk
        } finally {
            setSubmitting(false);
        }
    };

    const handleRenameNetwork = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(renameNetwork({
                oldName: dialogs.renameNetwork.ssid,
                newName: dialogs.renameNetwork.newName
            })).unwrap();
            dispatch(closeNetworkRenameNetworkDialog());
        } catch (error) {
            // Error handling is done in the thunk
        } finally {
            setSubmitting(false);
        }
    };

    const handleCloseAddDialog = (): void => {
        dispatch(closeNetworkAddNetworkDialog());
        setNewNetwork({ ssid: '', password: '' });
    };

    const handleClickShowPassword = () => {
        setShowPassword(!showPassword);
    };

    const handleMouseDownPassword = (
        event: React.MouseEvent<HTMLButtonElement>
    ) => {
        event.preventDefault();
    };

    return (
        <>
            {/* Add Network Dialog */}
            <ShareframeDialog
                open={dialogs.addNetwork.open}
                title={"Neues Netzwerk hinzufügen"}
                onClose={handleCloseAddDialog}
                onConfirm={handleSubmitNetwork}
                confirmDisabled={isActionsDisabled}
                confirmText="Hinzufügen"
                cancelText="Abbrechen"
                fullWidth={true}
                showActions={true}
            >
                <TextField
                    autoFocus
                    margin="dense"
                    id="ssid"
                    label="Netzwerkname (SSID)"
                    type="text"
                    fullWidth
                    variant="outlined"
                    value={newNetwork.ssid}
                    onChange={(e) => setNewNetwork({ ...newNetwork, ssid: e.target.value })}
                />
                <TextField
                    margin="dense"
                    id="password"
                    label="Passwort"
                    type={showPassword ? "text" : "password"}
                    fullWidth
                    variant="outlined"
                    value={newNetwork.password}
                    onChange={(e) => setNewNetwork({ ...newNetwork, password: e.target.value })}
                    InputProps={{
                        endAdornment: (
                            <InputAdornment position="end">
                                <IconButton
                                    aria-label="toggle current password visibility"
                                    onClick={handleClickShowPassword}
                                    onMouseDown={handleMouseDownPassword}
                                    edge="end"
                                >
                                    {showPassword ? <VisibilityOff /> : <Visibility />}
                                </IconButton>
                            </InputAdornment>
                        ),
                    }}
                />
            </ShareframeDialog>

            {/* Forget Network Dialog */}
            <ShareframeDialog
                open={dialogs.forgetNetwork.open}
                title="Netzwerk entfernen"
                onClose={() => dispatch(closeNetworkForgetNetworkDialog())}
                onConfirm={handleForgetNetwork}
                confirmText="Entfernen"
                cancelText="Abbrechen"
                confirmDisabled={isActionsDisabled}
            >
                <Typography variant="body1" gutterBottom>
                    Möchtest du das Netzwerk <Typography color='primary' component="b">{dialogs.forgetNetwork.ssid}</Typography> wirklich aus den gespeicherten Netzwerken entfernen?
                </Typography>
                <Typography variant="body1" color="text.secondary">
                    Du kannst das Netzwerk jederzeit erneut hinzufügen.
                </Typography>
            </ShareframeDialog>

            {/* Rename Network Dialog */}
            <ShareframeDialog
                open={dialogs.renameNetwork.open}
                title="Netzwerk umbenennen"
                onClose={() => dispatch(closeNetworkRenameNetworkDialog())}
                onConfirm={handleRenameNetwork}
                confirmText="Umbenennen"
                cancelText="Abbrechen"
                confirmDisabled={isActionsDisabled || !dialogs.renameNetwork.newName.trim()}
                fullWidth={true}
                showActions={true}
            >
                <Typography variant="body2" gutterBottom>
                    Aktueller Name: <Typography color='primary' component="span">{dialogs.renameNetwork.ssid}</Typography>
                </Typography>
                <TextField
                    autoFocus
                    margin="dense"
                    id="newName"
                    label="Neuer Netzwerkname"
                    type="text"
                    fullWidth
                    variant="outlined"
                    value={dialogs.renameNetwork.newName}
                    onChange={(e) => dispatch(updateNetworkRenameNetworkName(e.target.value))}
                />
                <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    Voreingestellte Netzwerke können nicht umbenannt werden.
                </Typography>
            </ShareframeDialog>
        </>
    );
};

export default Dialogs;