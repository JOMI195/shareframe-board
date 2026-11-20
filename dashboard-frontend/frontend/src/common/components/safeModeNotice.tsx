import { useState } from 'react';
import { Alert, AlertTitle, Box, Button } from '@mui/material';
import DesktopAccessDisabledIcon from '@mui/icons-material/DesktopAccessDisabled';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { exitSafeMode } from '@/store/piPower/piPower.Slice';

const REASON: Record<string, string> = {
    'remote': 'Der ShareFrame-Support hat die Anzeige vorübergehend ausgeschaltet.',
    'display-crash': 'Die Anzeige ist mehrmals abgestürzt und bleibt deshalb bis zum nächsten Neustart aus.',
    'boot-loop': 'Der Bilderrahmen ist mehrmals hintereinander neu gestartet, deshalb bleibt die Anzeige vorerst aus.',
};

// The panel stays blank in safe mode; without this the owner would only see a dead frame.
const SafeModeNotice = () => {
    const dispatch = useAppDispatch();
    const { safe_mode } = useAppSelector(selectConnectionMode);
    const [busy, setBusy] = useState(false);
    if (!safe_mode) return null;

    const turnOn = async () => {
        setBusy(true);
        await dispatch(exitSafeMode());
        setBusy(false);
    };

    return (
        <Alert
            role="region"
            aria-labelledby="safe-mode-title"
            severity="warning"
            icon={<DesktopAccessDisabledIcon fontSize="inherit" />}
            sx={{ width: '100%', mb: 2 }}
        >
            <AlertTitle id="safe-mode-title" sx={{ mb: 0.25 }}>{'Anzeige ausgeschaltet'}</AlertTitle>
            {REASON[safe_mode] ?? 'Die Anzeige des Bilderrahmens ist ausgeschaltet.'}
            {/* Below the text, so it keeps its full width on a phone. */}
            <Box sx={{ mt: 1.5 }}>
                <Button variant="outlined" color="inherit" size="small" onClick={turnOn} loading={busy}>
                    {'Anzeige wieder einschalten'}
                </Button>
            </Box>
        </Alert>
    );
};

export default SafeModeNotice;
