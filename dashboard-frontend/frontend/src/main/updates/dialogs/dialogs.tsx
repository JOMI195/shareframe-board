import ShareframeDialog from "@/common/components/shareframeDialog";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeUpdatesConfirmUpdateDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { performUpdate } from "@/store/updates/updates.Slice";
import { Stack, Typography } from "@mui/material";
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
        <>
            <ShareframeDialog
                open={isUpdateConfirmDialogOpen}
                title="Neue Version installieren"
                onClose={() => dispatch(closeUpdatesConfirmUpdateDialog())}
                onConfirm={handleConfirmUpdate}
                confirmText="Installieren"
                cancelText="Abbrechen"
            >
                <Stack spacing={2}>
                    <Typography variant="body1" gutterBottom>
                        Möchtest du das neue Update wirklich installieren?
                    </Typography>
                    <Typography variant="body1">
                        Zum Start der Installation wirst du zunächst ausgeloggt. Anschließend wird die Bilderwiedergabe sowie dieses Dashboard kurzzeitig beendet und nicht zur Verfügung stehen.
                    </Typography>
                    <Typography variant="body1">
                        Nach erfolgreicher Installation startet der Bilderrahmen neu und diese Seite muss für den Zugang zum Dashboard neu geladen werden. Schlägt das Update fehl, stellt der Bilderrahmen automatisch die vorherige Version wieder her.
                    </Typography>
                </Stack>

            </ShareframeDialog>
        </>
    );
}

export default Dialogs;