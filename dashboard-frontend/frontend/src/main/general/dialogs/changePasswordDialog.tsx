import React, { useState } from 'react';
import { IconButton, InputAdornment, TextField, Typography } from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import FormDialog from '@/common/components/formDialog';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import { closeGeneralChangePasswordDialog, getDialogs } from '@/store/dialogs/dialogs.Slice';
import { changePasswordThunk } from '@/store/auth/auth.Slice';

const MIN_PASSWORD_LENGTH = 8;
const MAX_PASSWORD_LENGTH = 63;

const ChangePasswordDialog: React.FC = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { open } = useAppSelector(getDialogs).general.changePassword;

    const [currentPassword, setCurrentPassword] = useState('');
    const [newPassword, setNewPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [submitting, setSubmitting] = useState(false);
    const [showPasswords, setShowPasswords] = useState(false);

    const newPasswordTooShort = newPassword.length > 0 && newPassword.length < MIN_PASSWORD_LENGTH;
    const newPasswordTooLong = newPassword.length > MAX_PASSWORD_LENGTH;
    const passwordsMismatch = confirmPassword.length > 0 && newPassword !== confirmPassword;
    const canSubmit = currentPassword.length > 0
        && newPassword.length >= MIN_PASSWORD_LENGTH
        && newPassword.length <= MAX_PASSWORD_LENGTH
        && newPassword === confirmPassword
        && isConnected;

    const resetFields = () => {
        setCurrentPassword('');
        setNewPassword('');
        setConfirmPassword('');
        setShowPasswords(false);
    };

    const handleClose = (): void => {
        if (submitting) return;
        dispatch(closeGeneralChangePasswordDialog());
        resetFields();
    };

    const handleSubmit = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(changePasswordThunk({ currentPassword, newPassword })).unwrap();
            dispatch(closeGeneralChangePasswordDialog());
            resetFields();
        } catch {
            // The thunk reports the failure; the input stays for another try.
        } finally {
            setSubmitting(false);
        }
    };

    const visibilityAdornment = (
        <InputAdornment position="end">
            <IconButton
                aria-label={showPasswords ? "Passwörter verbergen" : "Passwörter anzeigen"}
                onClick={() => setShowPasswords((s) => !s)}
                onMouseDown={(e) => e.preventDefault()}
                edge="end"
            >
                {showPasswords ? <VisibilityOff /> : <Visibility />}
            </IconButton>
        </InputAdornment>
    );

    return (
        <FormDialog
            open={open}
            title="Geräte-Passwort ändern"
            submitLabel="Passwort ändern"
            onSubmit={handleSubmit}
            onClose={handleClose}
            submitDisabled={!canSubmit}
            loading={submitting}
        >
            <Typography variant="body2" sx={{ color: "text.secondary" }}>
                Mit diesem Passwort meldest du dich auch ohne Internet in den Einstellungen an.
            </Typography>
            <TextField
                autoFocus
                margin="dense"
                label="Aktuelles Passwort"
                type={showPasswords ? "text" : "password"}
                autoComplete="current-password"
                fullWidth
                value={currentPassword}
                onChange={(e) => setCurrentPassword(e.target.value)}
                slotProps={{ input: { endAdornment: visibilityAdornment } }}
            />
            <TextField
                margin="dense"
                label="Neues Passwort"
                type={showPasswords ? "text" : "password"}
                autoComplete="new-password"
                fullWidth
                value={newPassword}
                onChange={(e) => setNewPassword(e.target.value)}
                error={newPasswordTooShort || newPasswordTooLong}
                helperText="8 bis 63 Zeichen"
            />
            <TextField
                margin="dense"
                label="Neues Passwort bestätigen"
                type={showPasswords ? "text" : "password"}
                autoComplete="new-password"
                fullWidth
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                error={passwordsMismatch}
                helperText={passwordsMismatch ? "Passwörter stimmen nicht überein" : " "}
            />
        </FormDialog>
    );
};

export default ChangePasswordDialog;
