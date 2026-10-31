import ConfirmDialog from "@/common/components/confirmDialog";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeUpdatesConfirmUpdateDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { performUpdate } from "@/store/updates/updates.Slice";
import { useNavigate } from "react-router";

const Dialogs = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();

    const isUpdateConfirmDialogOpen = useAppSelector(getDialogs).updates.confirmUpdate.open;

    const handleConfirmUpdate = async () => {
        dispatch(closeUpdatesConfirmUpdateDialog());
        await dispatch(performUpdate(navigate));
    };

    return (
        <ConfirmDialog
            open={isUpdateConfirmDialogOpen}
            title="Neue Version installieren?"
            confirmLabel="Installieren"
            onConfirm={handleConfirmUpdate}
            onCancel={() => dispatch(closeUpdatesConfirmUpdateDialog())}
        >
            Du wirst abgemeldet, die Diashow und die Einstellungen sind kurz nicht erreichbar. Danach startet der Bilderrahmen neu und du lädst diese Seite neu. Schlägt das Update fehl, stellt er die vorherige Version wieder her.
        </ConfirmDialog>
    );
}

export default Dialogs;
