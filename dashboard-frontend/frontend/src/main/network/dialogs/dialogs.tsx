import React, { useState } from 'react';
import {
    Alert, DialogContentText, Typography, TextField, IconButton, InputAdornment
} from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import ConfirmDialog from '@/common/components/confirmDialog';
import FormDialog from '@/common/components/formDialog';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    closeNetworkAddNetworkDialog,
    closeNetworkChangeApPasswordDialog,
    closeNetworkConnectDialog,
    closeNetworkForgetNetworkDialog,
    getDialogs
} from '@/store/dialogs/dialogs.Slice';
import {
    addNetwork,
    changeApPassword,
    connectToSavedNetworks,
    forgetNetwork,
    NetworkCredentials,
    selectNetworkState
} from '@/store/network/network.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { boardAddress } from '@/common/utils/boardAddress';

const AP_PASSWORD_MIN = 8;
const AP_PASSWORD_MAX = 63;
// hostapd only accepts printable-ASCII WPA2 passphrases.
const AP_PASSWORD_PATTERN = /^[\x20-\x7e]*$/;

const Dialogs: React.FC = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { loading } = useAppSelector(selectNetworkState);
    const { mode, ap_ssid } = useAppSelector(selectConnectionMode);
    const dialogs = useAppSelector(getDialogs).network;

    const [newNetwork, setNewNetwork] = useState<NetworkCredentials>({ ssid: '', password: '' });
    const [submitting, setSubmitting] = useState<boolean>(false);
    const [showPassword, setShowPassword] = useState(false);
    const [apPassword, setApPassword] = useState('');
    const [apPasswordConfirm, setApPasswordConfirm] = useState('');


    const apPasswordInvalidChars = !AP_PASSWORD_PATTERN.test(apPassword);
    const apPasswordMismatch = apPasswordConfirm.length > 0 && apPassword !== apPasswordConfirm;
    const canSubmitApPassword = apPassword.length >= AP_PASSWORD_MIN
        && apPassword.length <= AP_PASSWORD_MAX
        && !apPasswordInvalidChars
        && apPassword === apPasswordConfirm
        && !loading && isConnected;

    const handleSubmitNetwork = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(addNetwork(newNetwork)).unwrap();
            dispatch(closeNetworkAddNetworkDialog());
            setNewNetwork({ ssid: '', password: '' });
        } catch {
            // The thunk reports the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const handleForgetNetwork = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(forgetNetwork(dialogs.forgetNetwork.ssid)).unwrap();
            dispatch(closeNetworkForgetNetworkDialog());
        } catch {
            // The thunk reports the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const handleCloseAddDialog = (): void => {
        if (submitting) return;
        dispatch(closeNetworkAddNetworkDialog());
        setNewNetwork({ ssid: '', password: '' });
    };

    const handleCloseApPasswordDialog = (): void => {
        if (submitting) return;
        dispatch(closeNetworkChangeApPasswordDialog());
        setApPassword('');
        setApPasswordConfirm('');
    };

    const handleSubmitApPassword = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(changeApPassword(apPassword)).unwrap();
            dispatch(closeNetworkChangeApPasswordDialog());
            setApPassword('');
            setApPasswordConfirm('');
        } catch {
            // The thunk reports the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const handleConnect = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(connectToSavedNetworks()).unwrap();
            dispatch(closeNetworkConnectDialog());
        } catch {
            // The thunk reports the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const handleClickShowPassword = () => {
        setShowPassword(!showPassword);
    };

    const passwordVisibilityAdornment = (
        <InputAdornment position="end">
            <IconButton
                aria-label={showPassword ? "Passwort verbergen" : "Passwort anzeigen"}
                onClick={handleClickShowPassword}
                onMouseDown={(e) => e.preventDefault()}
                edge="end"
            >
                {showPassword ? <VisibilityOff /> : <Visibility />}
            </IconButton>
        </InputAdornment>
    );

    return (
        <>
            <FormDialog
                open={dialogs.addNetwork.open}
                title="WLAN hinzufügen"
                submitLabel="Hinzufügen"
                onSubmit={handleSubmitNetwork}
                onClose={handleCloseAddDialog}
                submitDisabled={loading || !isConnected}
                loading={submitting}
            >
                <TextField
                    autoFocus
                    margin="dense"
                    id="ssid"
                    label="Netzwerkname (SSID)"
                    type="text"
                    autoComplete="off"
                    autoCapitalize="none"
                    fullWidth
                    value={newNetwork.ssid}
                    onChange={(e) => setNewNetwork({ ...newNetwork, ssid: e.target.value })}
                    slotProps={{ htmlInput: { autoCorrect: 'off', spellCheck: false } }}
                />
                <TextField
                    margin="dense"
                    id="password"
                    label="Passwort"
                    type={showPassword ? "text" : "password"}
                    autoComplete="off"
                    fullWidth
                    value={newNetwork.password}
                    onChange={(e) => setNewNetwork({ ...newNetwork, password: e.target.value })}
                    slotProps={{ input: { endAdornment: passwordVisibilityAdornment } }}
                />
            </FormDialog>

            <ConfirmDialog
                open={dialogs.forgetNetwork.open}
                title={`„${dialogs.forgetNetwork.ssid}“ entfernen?`}
                confirmLabel="Entfernen"
                onConfirm={handleForgetNetwork}
                onCancel={() => { if (!submitting) dispatch(closeNetworkForgetNetworkDialog()); }}
                confirmDisabled={loading || !isConnected}
                loading={submitting}
                destructive
            >
                Du kannst das WLAN jederzeit wieder hinzufügen.
            </ConfirmDialog>

            <ConfirmDialog
                open={dialogs.connect.open}
                title="Mit WLAN verbinden?"
                confirmLabel="Verbinden"
                onConfirm={handleConnect}
                onCancel={() => { if (!submitting) dispatch(closeNetworkConnectDialog()); }}
                confirmDisabled={loading || !isConnected}
                loading={submitting}
            >
                <DialogContentText gutterBottom>
                    Der Rahmen beendet den Hotspot und versucht etwa eine Minute lang, sich mit einem
                    gespeicherten WLAN zu verbinden. Dein Gerät verliert dabei die Verbindung zu dieser Seite.
                </DialogContentText>
                <DialogContentText variant="body2">
                    Klappt es, verbinde dein Gerät wieder mit deinem WLAN und öffne{' '}
                    <b>{boardAddress(ap_ssid)}</b>. Klappt es nicht, erscheint der Hotspot wieder.
                    Verbinde dich erneut damit und prüfe die Zugangsdaten.
                </DialogContentText>
            </ConfirmDialog>

            <FormDialog
                open={dialogs.changeApPassword.open}
                title="Hotspot-Passwort ändern"
                submitLabel="Passwort ändern"
                onSubmit={handleSubmitApPassword}
                onClose={handleCloseApPasswordDialog}
                submitDisabled={!canSubmitApPassword}
                loading={submitting}
            >
                <Typography variant="body2" sx={{ color: "text.secondary" }}>
                    Passwort des WLAN-Hotspots, den der Bilderrahmen öffnet, wenn er kein
                    bekanntes WLAN findet.
                </Typography>
                {mode === 'ap' && (
                    <Alert severity="warning">
                        Du bist mit dem Hotspot verbunden. Nach der Änderung wird die Verbindung
                        getrennt und du verbindest dich mit dem neuen Passwort neu.
                    </Alert>
                )}
                <TextField
                    autoFocus
                    margin="dense"
                    label="Neues Hotspot-Passwort"
                    type={showPassword ? "text" : "password"}
                    autoComplete="new-password"
                    fullWidth
                    value={apPassword}
                    onChange={(e) => setApPassword(e.target.value)}
                    error={apPasswordInvalidChars}
                    helperText={apPasswordInvalidChars
                        ? "Nur ASCII-Zeichen erlaubt (keine Umlaute)"
                        : "8 bis 63 Zeichen, nur ASCII"}
                    slotProps={{ input: { endAdornment: passwordVisibilityAdornment } }}
                />
                <TextField
                    margin="dense"
                    label="Neues Hotspot-Passwort bestätigen"
                    type={showPassword ? "text" : "password"}
                    autoComplete="new-password"
                    fullWidth
                    value={apPasswordConfirm}
                    onChange={(e) => setApPasswordConfirm(e.target.value)}
                    error={apPasswordMismatch}
                    helperText={apPasswordMismatch ? "Passwörter stimmen nicht überein" : " "}
                />
            </FormDialog>
        </>
    );
};

export default Dialogs;
