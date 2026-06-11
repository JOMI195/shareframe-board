import React, { useState } from 'react';
import {
    Alert, Typography, TextField, IconButton, InputAdornment
} from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import ShareframeDialog from '@/common/components/shareframeDialog';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    closeNetworkAddNetworkDialog,
    closeNetworkChangeApPasswordDialog,
    closeNetworkForgetNetworkDialog,
    getDialogs
} from '@/store/dialogs/dialogs.Slice';
import {
    addNetwork,
    changeApPassword,
    forgetNetwork,
    NetworkCredentials,
    selectNetworkState
} from '@/store/network/network.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';

const AP_PASSWORD_MIN = 8;
const AP_PASSWORD_MAX = 63;
// hostapd only accepts printable-ASCII WPA2 passphrases.
const AP_PASSWORD_PATTERN = /^[\x20-\x7e]*$/;

const Dialogs: React.FC = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { loading } = useAppSelector(selectNetworkState);
    const { mode } = useAppSelector(selectConnectionMode);
    const dialogs = useAppSelector(getDialogs).network;

    const [newNetwork, setNewNetwork] = useState<NetworkCredentials>({ ssid: '', password: '' });
    const [submitting, setSubmitting] = useState<boolean>(false);
    const [showPassword, setShowPassword] = useState(false);
    const [apPassword, setApPassword] = useState('');
    const [apPasswordConfirm, setApPasswordConfirm] = useState('');

    const isActionsDisabled = submitting || loading || !isConnected;

    const apPasswordInvalidChars = !AP_PASSWORD_PATTERN.test(apPassword);
    const apPasswordMismatch = apPasswordConfirm.length > 0 && apPassword !== apPasswordConfirm;
    const canSubmitApPassword = apPassword.length >= AP_PASSWORD_MIN
        && apPassword.length <= AP_PASSWORD_MAX
        && !apPasswordInvalidChars
        && apPassword === apPasswordConfirm
        && !isActionsDisabled;

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

    const handleCloseAddDialog = (): void => {
        dispatch(closeNetworkAddNetworkDialog());
        setNewNetwork({ ssid: '', password: '' });
    };

    const handleCloseApPasswordDialog = (): void => {
        dispatch(closeNetworkChangeApPasswordDialog());
        setApPassword('');
        setApPasswordConfirm('');
    };

    const handleSubmitApPassword = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(changeApPassword(apPassword)).unwrap();
            handleCloseApPasswordDialog();
        } catch (error) {
            // Error handling is done in the thunk
        } finally {
            setSubmitting(false);
        }
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

            {/* Change AP Password Dialog */}
            <ShareframeDialog
                open={dialogs.changeApPassword.open}
                title="AP-Passwort ändern"
                onClose={handleCloseApPasswordDialog}
                onConfirm={handleSubmitApPassword}
                confirmDisabled={!canSubmitApPassword}
                confirmText="Ändern"
                cancelText="Abbrechen"
                fullWidth={true}
                showActions={true}
            >
                <Typography variant="body2" color="text.secondary" gutterBottom>
                    Passwort des WLAN-Hotspots, den der Bilderrahmen öffnet, wenn er kein
                    bekanntes Netzwerk findet.
                </Typography>
                {mode === 'ap' && (
                    <Alert severity="warning" sx={{ mb: 1 }}>
                        Du bist über den AP verbunden – nach der Änderung wird die Verbindung
                        getrennt und du musst dich mit dem neuen Passwort neu verbinden.
                    </Alert>
                )}
                <TextField
                    autoFocus
                    margin="dense"
                    label="Neues AP-Passwort"
                    type={showPassword ? "text" : "password"}
                    fullWidth
                    variant="outlined"
                    value={apPassword}
                    onChange={(e) => setApPassword(e.target.value)}
                    error={apPasswordInvalidChars}
                    helperText={apPasswordInvalidChars
                        ? "Nur ASCII-Zeichen erlaubt (keine Umlaute)"
                        : "8–63 Zeichen, nur ASCII"}
                    InputProps={{
                        endAdornment: (
                            <InputAdornment position="end">
                                <IconButton
                                    aria-label="Passwort anzeigen"
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
                <TextField
                    margin="dense"
                    label="Bestätigen"
                    type={showPassword ? "text" : "password"}
                    fullWidth
                    variant="outlined"
                    value={apPasswordConfirm}
                    onChange={(e) => setApPasswordConfirm(e.target.value)}
                    error={apPasswordMismatch}
                    helperText={apPasswordMismatch ? "Passwörter stimmen nicht überein" : " "}
                />
            </ShareframeDialog>
        </>
    );
};

export default Dialogs;