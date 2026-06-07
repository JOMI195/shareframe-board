import ShareframeDialog from "@/common/components/shareframeDialog";
import { usePiConnection } from "@/context/piConnection/piConnectionContext";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeRestartDialog, closeShutdownDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { restartPi, shutdownPi } from "@/store/piPower/piPower.Slice";
import { Stack, Typography } from "@mui/material";
import { useNavigate } from "react-router";

const Dialogs = () => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();

    const isRestartDialogOpen = useAppSelector(getDialogs).general.restart.open;
    const isShutdownDialogOpen = useAppSelector(getDialogs).general.shutdown.open;

    const handleConfirmRestart = async () => {
        dispatch(closeRestartDialog())
        await dispatch(restartPi(navigate));
    };

    const handleConfirmShutdown = async () => {
        dispatch(closeShutdownDialog())
        await dispatch(shutdownPi(navigate));
    };

    return (
        <>
            <ShareframeDialog
                open={isRestartDialogOpen}
                title="Bilderrahmen neustarten"
                onClose={() => dispatch(closeRestartDialog())}
                onConfirm={handleConfirmRestart}
                confirmText="Neustarten"
                cancelText="Abbrechen"
                confirmDisabled={!isConnected}
            >
                <Stack spacing={2}>
                    <Typography variant="body1" gutterBottom>
                        Möchtest du den Bilderrahmen wirklich neustarten?
                    </Typography>
                    <Typography variant="body1">
                        Während dem Neustart ist das Dashboard nicht verfügbar.
                    </Typography>
                </Stack>
            </ShareframeDialog>

            <ShareframeDialog
                open={isShutdownDialogOpen}
                title="Bilderrahmen herunterfahren"
                onClose={() => dispatch(closeShutdownDialog())}
                onConfirm={handleConfirmShutdown}
                confirmText="Herunterfahren"
                cancelText="Abbrechen"
                confirmDisabled={!isConnected}
            >
                <Stack spacing={2}>
                    <Typography variant="body1" gutterBottom>
                        Möchtest du den Bilderrahmen wirklich herunterfahren?
                    </Typography>
                    <Typography variant="body1">
                        Nach dem Herunterfahren ist das Dashboard nicht mehr verfügbar und die Bildwiedergabe deaktiviert.
                    </Typography>
                    <Typography variant="body1">
                        Um ihn anschließend neu zu starten musst du die Stromzufuhr unterbrechen und wieder herstellen.
                    </Typography>
                </Stack>
            </ShareframeDialog>

        </>
    );
}

export default Dialogs;