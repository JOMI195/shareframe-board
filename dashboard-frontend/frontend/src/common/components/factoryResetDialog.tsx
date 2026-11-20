import { useEffect, useState } from "react";
import { Alert, DialogContentText, Stack, TextField, Typography } from "@mui/material";
import ConfirmDialog from "@/common/components/confirmDialog";
import FormDialog from "@/common/components/formDialog";
import { usePiConnection } from "@/context/piConnection/piConnectionContextValue";
import { useAppDispatch, useAppSelector } from "@/store";
import { closeGeneralFactoryResetDialog, getDialogs } from "@/store/dialogs/dialogs.Slice";
import { selectConnectionMode } from "@/store/connectionMode/connectionMode.Slice";
import { cancelResetCode, factoryResetPi, requestResetCode } from "@/store/piPower/piPower.Slice";
import { useNavigate } from "react-router";

const WIPED = "Alle Fotos, gespeicherten WLANs, das Geräte-Passwort und alle Einstellungen werden endgültig gelöscht. Danach startet der Bilderrahmen im Einrichtungsmodus und sein Hotspot nutzt wieder das ursprüngliche Passwort.";

const formatWait = (secs: number) => `${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, "0")}`;

// "forgot" sits under the sign-in and first offers the way that keeps everything.
type Variant = "reset" | "forgot";

const FactoryResetDialog = ({ variant = "reset" }: { variant?: Variant }) => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();
    const isOpen = useAppSelector(getDialogs).general.factoryReset.open;
    const { mode } = useAppSelector(selectConnectionMode);
    const [step, setStep] = useState<"confirm" | "code">("confirm");
    const [busy, setBusy] = useState(false);
    const [code, setCode] = useState("");
    const [triesLeft, setTriesLeft] = useState<number | null>(null);
    const [codeDead, setCodeDead] = useState(false);
    const [showsAt, setShowsAt] = useState(0);
    const [expiresAt, setExpiresAt] = useState(0);
    const [now, setNow] = useState(() => Date.now());

    const waiting = step === "code" && isOpen;
    useEffect(() => {
        if (!waiting) return;
        const tick = setInterval(() => setNow(Date.now()), 1000);
        // Leaving the page must not leave the code on the frame for others to see.
        window.addEventListener("pagehide", cancelResetCode);
        return () => {
            clearInterval(tick);
            window.removeEventListener("pagehide", cancelResetCode);
        };
    }, [waiting]);

    const reset = () => {
        setStep("confirm");
        setCode("");
        setTriesLeft(null);
        setBusy(false);
    };

    const close = () => {
        if (busy) return;
        if (step === "code") cancelResetCode();
        dispatch(closeGeneralFactoryResetDialog());
        reset();
    };

    const showCode = async () => {
        setBusy(true);
        setCodeDead(false);
        const result = await dispatch(requestResetCode());
        setBusy(false);
        if (requestResetCode.fulfilled.match(result)) {
            setShowsAt(Date.now() + result.payload.showsInSecs * 1000);
            setExpiresAt(Date.now() + result.payload.expiresInSecs * 1000);
            setNow(Date.now());
            setStep("code");
        }
    };

    const submit = async () => {
        setBusy(true);
        const result = await dispatch(factoryResetPi({ navigate, code: code.replace(/\s/g, "") }));
        setBusy(false);
        if (factoryResetPi.fulfilled.match(result)) {
            dispatch(closeGeneralFactoryResetDialog());
            reset();
            return;
        }
        const left = result.payload?.triesLeft;
        if (left === undefined) return;
        setCode("");
        if (left > 0) {
            setTriesLeft(left);
        } else {
            reset();
            setCodeDead(true);
        }
    };

    const secondsUntilShown = Math.max(0, Math.ceil((showsAt - now) / 1000));
    const minutesValid = Math.max(0, Math.ceil((expiresAt - now) / 60000));
    const codeReady = code.replace(/\s/g, "").length === 6;
    const keepEverything = mode === "ap"
        ? "Melde dich mit dem Einrichtungscode vom Bildschirm des Bilderrahmens an und lege unter „System“ ein neues Passwort fest. So bleibt alles erhalten."
        : "Melde dich mit einem Einmal-Code von shareframe.de an und lege unter „System“ ein neues Passwort fest. So bleibt alles erhalten.";

    // Like the main app: the question is a small dialog, the code entry a task dialog (fullscreen on phones).
    return (
        <>
            <ConfirmDialog
                open={isOpen && step === "confirm"}
                title={variant === "forgot" ? "Passwort vergessen?" : "Auf Werkseinstellungen zurücksetzen?"}
                confirmLabel="Zurücksetzen"
                onConfirm={showCode}
                onCancel={close}
                confirmDisabled={!isConnected}
                loading={busy}
                destructive
            >
                <Stack spacing={2}>
                    {variant === "forgot" && <Alert severity="info">{keepEverything}</Alert>}
                    {codeDead && (
                        <Alert severity="warning">Der Code wurde zu oft falsch eingegeben. Fordere einen neuen an.</Alert>
                    )}
                    <DialogContentText>
                        {variant === "forgot" ? "Sonst bleibt nur das Zurücksetzen. " : ""}
                        {WIPED} Zur Bestätigung erscheint ein Code auf dem Bildschirm des Bilderrahmens.
                    </DialogContentText>
                </Stack>
            </ConfirmDialog>
            {/* Opens while the question closes: its own back-button entry would race the question's. */}
            <FormDialog
                open={isOpen && step === "code"}
                title="Code vom Bilderrahmen eingeben"
                submitLabel="Endgültig zurücksetzen"
                onSubmit={submit}
                onClose={close}
                submitDisabled={!codeReady || !isConnected}
                loading={busy}
                destructive
                backToClose={false}
            >
                <Typography variant="body2" role="status" sx={{ color: "text.secondary" }}>
                    {secondsUntilShown > 0
                        ? `Der Code erscheint in etwa ${formatWait(secondsUntilShown)} auf dem Bildschirm des Bilderrahmens.`
                        : "Der Code steht jetzt auf dem Bildschirm des Bilderrahmens."}
                </Typography>
                {secondsUntilShown === 0 && minutesValid > 0 && (
                    <Typography variant="body1" color="error">
                        {`Noch ${minutesValid} ${minutesValid === 1 ? "Minute" : "Minuten"} gültig`}
                    </Typography>
                )}
                {triesLeft !== null && (
                    <Alert severity="error">
                        {`Der Code stimmt nicht. Noch ${triesLeft} ${triesLeft === 1 ? "Versuch" : "Versuche"}.`}
                    </Alert>
                )}
                <TextField
                    autoFocus
                    margin="dense"
                    label="Code"
                    fullWidth
                    value={code}
                    onChange={(e) => setCode(e.target.value)}
                    slotProps={{ htmlInput: { inputMode: "numeric", autoComplete: "one-time-code", maxLength: 7 } }}
                />
            </FormDialog>
        </>
    );
};

export default FactoryResetDialog;
