import ConfirmDialog from "@/common/components/confirmDialog";
import { usePiConnection } from "@/context/piConnection/piConnectionContextValue";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeGeneralFactoryResetDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { factoryResetPi } from "@/store/piPower/piPower.Slice";
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
        <ConfirmDialog
            open={isOpen}
            title="Auf Werkseinstellungen zurücksetzen?"
            confirmLabel="Zurücksetzen"
            onConfirm={handleConfirm}
            onCancel={() => dispatch(closeGeneralFactoryResetDialog())}
            confirmDisabled={!isConnected}
            destructive
        >
            Alle Fotos, gespeicherten WLANs, das Geräte-Passwort und alle Einstellungen werden endgültig gelöscht. Danach startet der Bilderrahmen im Einrichtungsmodus und sein Hotspot nutzt wieder das ursprüngliche Passwort.
        </ConfirmDialog>
    );
};

export default FactoryResetDialog;
