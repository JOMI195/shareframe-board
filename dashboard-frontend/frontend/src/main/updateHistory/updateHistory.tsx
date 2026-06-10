import { useEffect, ReactNode } from 'react';
import { Stack, Chip, Typography, Box } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectUpdatesState,
    fetchUpdateHistory,
    UpdateHistoryEntry,
} from '@/store/updates/updates.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const DASH = '—';

const RESULT_CHIP: Record<string, { label: string; color: 'success' | 'warning' | 'error' }> = {
    'committed': { label: 'Erfolgreich', color: 'success' },
    'rolled-back': { label: 'Zurückgerollt', color: 'error' },
    'install-failed': { label: 'Installation fehlgeschlagen', color: 'error' },
};

const fmtTimestamp = (ts: string): string => {
    const d = new Date(ts);
    return isNaN(d.getTime()) ? ts || DASH : d.toLocaleString('de-DE');
};

const node = (value: ReactNode) => ({ type: 'reactNode' as const, value });

const entryNode = (entry: UpdateHistoryEntry) => {
    const chip = RESULT_CHIP[entry.result] ?? { label: entry.result, color: 'warning' as const };
    return node(
        <Box>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
                <Chip size="small" label={chip.label} color={chip.color} />
                <Typography variant="body1">
                    {(entry.from_version || DASH) + ' → ' + (entry.to_version || DASH)}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                    {fmtTimestamp(entry.timestamp)}
                </Typography>
            </Box>
            {entry.error && (
                <Typography variant="body2" color="error" sx={{ mt: 0.5 }}>
                    {entry.error}
                </Typography>
            )}
        </Box>
    );
};

const UpdateHistory = () => {
    const dispatch = useAppDispatch();
    const { history } = useAppSelector(selectUpdatesState);

    useEffect(() => {
        dispatch(fetchUpdateHistory());
    }, [dispatch]);

    return (
        <Stack width={"100%"} spacing={3}>
            <ShareframeInfoCard
                title="Update-Verlauf"
                sections={
                    history.length === 0
                        ? [{ content: "Noch keine Updates durchgeführt." }]
                        : history.map((entry) => ({ content: entryNode(entry) }))
                }
            />
        </Stack>
    );
};

export default UpdateHistory;
