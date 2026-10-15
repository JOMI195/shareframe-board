import ShareframeDialog from "@/common/components/shareframeDialog";
import { usePiConnection } from "@/context/piConnection/piConnectionContextValue";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeGeneralFactoryResetDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { factoryResetPi } from "@/store/piPower/piPower.Slice";
import { Stack, Typography } from "@mui/material";
import { useNavigate } from "react-router";

const FactoryResetDialog = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();
    const isOpen = useAppSelector(getDialogs).general.factoryReset.open;

    const handleConfirm = async () => {
        dispatch(closeGeneralFactoryResetDialog());
        await dispatch(factoryResetPi(navigate));
    };

    return (
        <ShareframeDialog
            open={isOpen}
            title="Auf Werkseinstellungen zurücksetzen"
            onClose={() => dispatch(closeGeneralFactoryResetDialog())}
            onConfirm={handleConfirm}
            confirmText="Zurücksetzen"
            cancelText="Abbrechen"
            confirmColor="error"
            confirmDisabled={!isConnected}
        >
            <Stack spacing={2}>
                <Typography variant="body1">
                    Möchtest du den Bilderrahmen wirklich auf Werkseinstellungen zurücksetzen?
                </Typography>
                <Typography variant="body1">
                    Alle Bilder, gespeicherten WLAN-Netzwerke, das Geräte-Passwort und alle Einstellungen werden gelöscht. Das kann nicht rückgängig gemacht werden.
                </Typography>
                <Typography variant="body1">
                    Danach startet der Bilderrahmen neu im Einrichtungsmodus und muss über sein WLAN neu eingerichtet werden.
                    Der Hotspot nutzt dann wieder sein ursprüngliches Passwort.
                </Typography>
            </Stack>
        </ShareframeDialog>
    );
};

export default FactoryResetDialog;
