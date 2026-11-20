import { useEffect, useRef, useState } from 'react';
import { Alert, Button, Stack, TextField, Typography } from '@mui/material';
import RestoreIcon from '@mui/icons-material/Restore';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { cancelResetCode, factoryResetPi, requestResetCode } from '@/store/piPower/piPower.Slice';
import { useNavigate } from 'react-router';

const WIPED = 'Alle Fotos, gespeicherten WLANs, das Geräte-Passwort und alle Einstellungen werden endgültig gelöscht. Danach startet der Bilderrahmen im Einrichtungsmodus und sein Hotspot nutzt wieder das ursprüngliche Passwort.';

const formatWait = (secs: number) => `${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, '0')}`;

interface Props {
    // Where "Abbrechen" leads; a code still on the panel is taken back first.
    onLeave: () => void;
    // Signed out the display cannot be switched on from here.
    signedIn: boolean;
    // When the page already offers its own way back before the reset starts.
    cancelOnlyOnceStarted?: boolean;
    // In a wide card the buttons sit on the right instead of spanning it.
    actionsAside?: boolean;
}

/** The reset as steps on a page: what gets lost, then the code the frame shows. */
const FactoryResetSteps = ({ onLeave, signedIn, cancelOnlyOnceStarted = false, actionsAside = false }: Props) => {
    const dispatch = useAppDispatch();
    const navigate = useNavigate();
    const { isConnected } = usePiConnection();
    const { safe_mode } = useAppSelector(selectConnectionMode);
    const [step, setStep] = useState<'confirm' | 'code'>('confirm');
    const [busy, setBusy] = useState(false);
    const [code, setCode] = useState('');
    const [triesLeft, setTriesLeft] = useState<number | null>(null);
    const [codeDead, setCodeDead] = useState(false);
    const [showsAt, setShowsAt] = useState(0);
    const [expiresAt, setExpiresAt] = useState(0);
    const [now, setNow] = useState(() => Date.now());

    // Leaving the page, by link, back button or closing the tab, must not leave the code on the frame.
    const codeShown = useRef(false);
    useEffect(() => {
        codeShown.current = step === 'code';
        if (step !== 'code') return;
        const tick = setInterval(() => setNow(Date.now()), 1000);
        window.addEventListener('pagehide', cancelResetCode);
        return () => {
            clearInterval(tick);
            window.removeEventListener('pagehide', cancelResetCode);
        };
    }, [step]);
    useEffect(() => () => {
        if (codeShown.current) cancelResetCode();
    }, []);

    const showCode = async () => {
        setBusy(true);
        setCodeDead(false);
        const result = await dispatch(requestResetCode());
        setBusy(false);
        if (requestResetCode.fulfilled.match(result)) {
            setShowsAt(Date.now() + result.payload.showsInSecs * 1000);
            setExpiresAt(Date.now() + result.payload.expiresInSecs * 1000);
            setNow(Date.now());
            setTriesLeft(null);
            setStep('code');
        }
    };

    const submit = async (e: React.FormEvent) => {
        e.preventDefault();
        setBusy(true);
        const result = await dispatch(factoryResetPi({ navigate, code: code.replace(/\s/g, '') }));
        setBusy(false);
        if (factoryResetPi.fulfilled.match(result)) {
            codeShown.current = false;
            return;
        }
        const left = result.payload?.triesLeft;
        if (left === undefined) return;
        setCode('');
        if (left > 0) {
            setTriesLeft(left);
        } else {
            codeShown.current = false;
            setStep('confirm');
            setCodeDead(true);
        }
    };

    const actions = actionsAside
        ? { direction: { xs: 'column', sm: 'row-reverse' } as const, sx: { gap: 1, '& > .MuiButton-root': { width: { xs: '100%', sm: 'auto' } } } }
        : { direction: 'column' as const, sx: { gap: 2, '& > .MuiButton-root': { width: '100%' } } };

    const leave = () => {
        if (codeShown.current) cancelResetCode();
        codeShown.current = false;
        onLeave();
    };

    if (step === 'confirm') {
        return (
            <Stack spacing={2}>
                {safe_mode && (
                    <Alert severity="warning">
                        {signedIn
                            ? 'Die Anzeige ist ausgeschaltet, deshalb kann dort kein Code erscheinen. Schalte sie zuerst über „Anzeige wieder einschalten“ auf der Startseite ein.'
                            : 'Die Anzeige ist ausgeschaltet, deshalb kann dort kein Code erscheinen. Melde dich an und schalte sie zuerst wieder ein.'}
                    </Alert>
                )}
                {codeDead && (
                    <Alert severity="warning">Der Code wurde zu oft falsch eingegeben. Fordere einen neuen an.</Alert>
                )}
                <Typography variant="body2">{WIPED}</Typography>
                <Typography variant="body2">
                    Zur Bestätigung erscheint ein Code auf dem Bildschirm des Bilderrahmens. Du brauchst ihn im nächsten Schritt.
                </Typography>
                <Stack direction={actions.direction} sx={actions.sx}>
                    <Button
                        variant="contained"
                        color="error"
                        startIcon={<RestoreIcon />}
                        onClick={showCode}
                        loading={busy}
                        disabled={!isConnected || !!safe_mode}
                    >
                        Code auf dem Bilderrahmen anzeigen
                    </Button>
                    {!cancelOnlyOnceStarted && <Button onClick={leave}>Abbrechen</Button>}
                </Stack>
            </Stack>
        );
    }

    const secondsUntilShown = Math.max(0, Math.ceil((showsAt - now) / 1000));
    const minutesValid = Math.max(0, Math.ceil((expiresAt - now) / 60000));
    const codeReady = code.replace(/\s/g, '').length === 6;

    return (
        <Stack component="form" spacing={2} onSubmit={submit} noValidate>
            <Typography variant="body2" role="status">
                {secondsUntilShown > 0
                    ? `Der Code erscheint in etwa ${formatWait(secondsUntilShown)} auf dem Bildschirm des Bilderrahmens.`
                    : 'Der Code steht jetzt auf dem Bildschirm des Bilderrahmens.'}
            </Typography>
            {secondsUntilShown === 0 && minutesValid > 0 && (
                <Typography variant="body2" color="error">
                    {`Noch ${minutesValid} ${minutesValid === 1 ? 'Minute' : 'Minuten'} gültig`}
                </Typography>
            )}
            {triesLeft !== null && (
                <Alert severity="error">
                    {`Der Code stimmt nicht. Noch ${triesLeft} ${triesLeft === 1 ? 'Versuch' : 'Versuche'}.`}
                </Alert>
            )}
            <TextField
                autoFocus
                label="Code"
                fullWidth
                value={code}
                onChange={(e) => setCode(e.target.value)}
                slotProps={{ htmlInput: { inputMode: 'numeric', autoComplete: 'one-time-code', maxLength: 7 } }}
            />
            <Stack direction={actions.direction} sx={actions.sx}>
                <Button type="submit" variant="contained" color="error" loading={busy} disabled={!codeReady || !isConnected}>
                    Endgültig zurücksetzen
                </Button>
                <Button onClick={leave} disabled={busy}>Abbrechen</Button>
            </Stack>
        </Stack>
    );
};

export default FactoryResetSteps;
