import { useEffect, ReactNode, Fragment } from 'react';
import { Stack, Chip, Typography, Box } from '@mui/material';
import Accordion from '@mui/material/Accordion';
import AccordionSummary from '@mui/material/AccordionSummary';
import AccordionDetails from '@mui/material/AccordionDetails';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
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

const EntryAccordion = ({ entry }: { entry: UpdateHistoryEntry }) => {
    const chip = RESULT_CHIP[entry.result] ?? { label: entry.result, color: 'warning' as const };
    const hasError = Boolean(entry.error);
    return (
        <Accordion
            disableGutters
            elevation={0}
            expanded={hasError ? undefined : false}
            sx={{ '&:before': { display: 'none' } }}
        >
            <AccordionSummary
                expandIcon={hasError ? <ExpandMoreIcon /> : null}
                sx={{ cursor: hasError ? 'pointer' : 'default !important', px: 0 }}
            >
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
                    <Chip size="small" label={chip.label} color={chip.color} />
                    <Typography variant="body1">
                        {(entry.from_version || DASH) + ' → ' + (entry.to_version || DASH)}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                        {fmtTimestamp(entry.timestamp)}
                    </Typography>
                </Box>
            </AccordionSummary>
            {hasError && (
                <AccordionDetails sx={{ px: 0, pt: 0 }}>
                    <Typography variant="body2" color="error">
                        {entry.error}
                    </Typography>
                </AccordionDetails>
            )}
        </Accordion>
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
                        : [{ content: node(<Box>{history.map((e, i) => <Fragment key={i}><EntryAccordion entry={e} /></Fragment>)}</Box>) }]
                }
            />
        </Stack>
    );
};

export default UpdateHistory;
