import React, { useState } from 'react';
import { IconButton, InputAdornment, TextField, Typography } from '@mui/material';
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import ShareframeDialog from '@/common/components/shareframeDialog';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
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
        && !submitting && isConnected;

    const resetFields = () => {
        setCurrentPassword('');
        setNewPassword('');
        setConfirmPassword('');
        setShowPasswords(false);
    };

    const handleClose = (): void => {
        dispatch(closeGeneralChangePasswordDialog());
        resetFields();
    };

    const handleSubmit = async (): Promise<void> => {
        setSubmitting(true);
        try {
            await dispatch(changePasswordThunk({ currentPassword, newPassword })).unwrap();
            dispatch(closeGeneralChangePasswordDialog());
            resetFields();
        } catch (error) {
            // Error handling is done in the thunk
        } finally {
            setSubmitting(false);
        }
    };

    const visibilityAdornment = (
        <InputAdornment position="end">
            <IconButton
                aria-label="Passwörter anzeigen"
                onClick={() => setShowPasswords((s) => !s)}
                onMouseDown={(e) => e.preventDefault()}
                edge="end"
            >
                {showPasswords ? <VisibilityOff /> : <Visibility />}
            </IconButton>
        </InputAdornment>
    );

    return (
        <ShareframeDialog
            open={open}
            title="Geräte-Passwort ändern"
            onClose={handleClose}
            onConfirm={handleSubmit}
            confirmDisabled={!canSubmit}
            confirmText="Ändern"
            cancelText="Abbrechen"
            fullWidth={true}
            showActions={true}
        >
            <Typography variant="body2" color="text.secondary" gutterBottom>
                Mit diesem Passwort kannst du dich auch ohne Internetverbindung am Dashboard anmelden.
            </Typography>
            <TextField
                autoFocus
                margin="dense"
                label="Aktuelles Passwort"
                type={showPasswords ? "text" : "password"}
                fullWidth
                variant="outlined"
                value={currentPassword}
                onChange={(e) => setCurrentPassword(e.target.value)}
                InputProps={{ endAdornment: visibilityAdornment }}
            />
            <TextField
                margin="dense"
                label="Neues Passwort"
                type={showPasswords ? "text" : "password"}
                fullWidth
                variant="outlined"
                value={newPassword}
                onChange={(e) => setNewPassword(e.target.value)}
                error={newPasswordTooShort || newPasswordTooLong}
                helperText="8 bis 63 Zeichen"
            />
            <TextField
                margin="dense"
                label="Neues Passwort bestätigen"
                type={showPasswords ? "text" : "password"}
                fullWidth
                variant="outlined"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                error={passwordsMismatch}
                helperText={passwordsMismatch ? "Passwörter stimmen nicht überein" : " "}
            />
        </ShareframeDialog>
    );
};

export default ChangePasswordDialog;
