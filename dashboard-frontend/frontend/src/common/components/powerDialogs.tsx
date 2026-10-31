import ConfirmDialog from "@/common/components/confirmDialog";
import { usePiConnection } from "@/context/piConnection/piConnectionContextValue";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeRestartDialog, closeShutdownDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { restartPi, shutdownPi } from "@/store/piPower/piPower.Slice";
import { useNavigate } from "react-router";

// Rendered once in MainLayout so the power menu can open them from any page.
const PowerDialogs = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();

    const isRestartDialogOpen = useAppSelector(getDialogs).general.restart.open;
    const isShutdownDialogOpen = useAppSelector(getDialogs).general.shutdown.open;

    const handleConfirmRestart = async () => {
        dispatch(closeRestartDialog());
        await dispatch(restartPi(navigate));
    };

    const handleConfirmShutdown = async () => {
        dispatch(closeShutdownDialog());
        await dispatch(shutdownPi(navigate));
    };

    return (
        <>
            <ConfirmDialog
                open={isRestartDialogOpen}
                title="Bilderrahmen neu starten?"
                confirmLabel="Neu starten"
                onConfirm={handleConfirmRestart}
                onCancel={() => dispatch(closeRestartDialog())}
                confirmDisabled={!isConnected}
            >
                Während des Neustarts sind die Einstellungen kurz nicht erreichbar.
            </ConfirmDialog>

            <ConfirmDialog
                open={isShutdownDialogOpen}
                title="Bilderrahmen herunterfahren?"
                confirmLabel="Herunterfahren"
                onConfirm={handleConfirmShutdown}
                onCancel={() => dispatch(closeShutdownDialog())}
                confirmDisabled={!isConnected}
                destructive
            >
                Die Diashow und die Einstellungen sind danach aus. Zum Einschalten trenne den Strom kurz und stecke ihn wieder ein.
            </ConfirmDialog>
        </>
    );
};

export default PowerDialogs;
