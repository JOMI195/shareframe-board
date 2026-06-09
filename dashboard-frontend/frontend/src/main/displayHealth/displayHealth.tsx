import { useEffect, ReactNode } from 'react';
import { Stack, Chip, LinearProgress, Box, Typography } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectDisplayStatsState,
    fetchDisplayStats,
    DisplayHealth,
} from '@/store/displayStats/displayStats.Slice';
import { selectFrameInfoState, fetchFrameInfos } from '@/store/frameInfo/frameInfo.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const STATS_POLL_MS = 5000;
const DASH = '—';

// --- formatting helpers -----------------------------------------------------
const fmtInt = (n?: number): string => (n === undefined || n === null ? DASH : n.toLocaleString('de-DE'));
const fmtMs = (n?: number): string => (n === undefined || n === null ? DASH : `${n.toLocaleString('de-DE')} ms`);

const fmtDuration = (ms?: number): string => {
    if (ms === undefined || ms === null) return DASH;
    const s = Math.floor(ms / 1000);
    const h = Math.floor(s / 3600);
    const m = Math.floor((s % 3600) / 60);
    const sec = s % 60;
    const parts: string[] = [];
    if (h) parts.push(`${h}h`);
    if (m || h) parts.push(`${m}m`);
    parts.push(`${sec}s`);
    return parts.join(' ');
};

const fmtUnix = (s?: number): string => {
    if (s === undefined || s === null || s === 0) return DASH;
    const d = new Date(s * 1000);
    return isNaN(d.getTime()) ? DASH : d.toLocaleString('de-DE');
};

const fmtWear = (p?: number): string => (p === undefined || p === null ? DASH : `${p.toFixed(4)} %`);

// ok / degraded / failed → coloured chip
const HEALTH_CHIP: Record<DisplayHealth, { label: string; color: 'success' | 'warning' | 'error' }> = {
    ok: { label: 'OK', color: 'success' },
    degraded: { label: 'Eingeschränkt', color: 'warning' },
    failed: { label: 'Ausgefallen', color: 'error' },
};

const node = (value: ReactNode) => ({ type: 'reactNode' as const, value });

const DisplayHealthPage = () => {
    const dispatch = useAppDispatch();
    const stats = useAppSelector(selectDisplayStatsState).displayStats;
    const info = useAppSelector(selectFrameInfoState).frameInfo;

    // Poll panel stats (and frame info for the device boot count) while open.
    useEffect(() => {
        dispatch(fetchDisplayStats());
        dispatch(fetchFrameInfos());
        const id = setInterval(() => {
            dispatch(fetchDisplayStats());
            dispatch(fetchFrameInfos());
        }, STATS_POLL_MS);
        return () => clearInterval(id);
    }, [dispatch]);

    const chip = stats?.health ? HEALTH_CHIP[stats.health] : undefined;
    const wear = stats?.wear_percent ?? 0;

    return (
        <Stack width={'100%'} spacing={3}>
            <ShareframeInfoCard
                title="Zustand"
                sections={[
                    {
                        label: 'Status',
                        content: node(
                            chip ? (
                                <Chip label={chip.label} color={chip.color} size="small" />
                            ) : (
                                <Typography variant="body2">{DASH}</Typography>
                            ),
                        ),
                    },
                    { label: 'Fehler in Folge', content: fmtInt(stats?.consecutive_failures) },
                    { label: 'Letzte Aktualisierung (Dauer)', content: fmtMs(stats?.epd_last_refresh_ms) },
                ]}
            />

            <ShareframeInfoCard
                title="Verschleiß"
                sections={[
                    {
                        label: `Aktualisierungen gesamt (von ${fmtInt(stats?.rated_refreshes)} erwartet)`,
                        content: node(
                            <Box>
                                <Typography variant="body2" gutterBottom>
                                    {fmtInt(stats?.epd_refresh_total)} ({fmtWear(stats?.wear_percent)})
                                </Typography>
                                <LinearProgress
                                    variant="determinate"
                                    value={Math.min(100, Math.max(0, wear))}
                                />
                            </Box>,
                        ),
                    },
                    { label: 'Bild-Aktualisierungen', content: fmtInt(stats?.epd_image_refresh_total) },
                    { label: 'Löschvorgänge', content: fmtInt(stats?.epd_clear_total) },
                ]}
            />

            <ShareframeInfoCard
                title="Zuverlässigkeit"
                sections={[
                    { label: 'Fehlgeschlagene Aktualisierungen', content: fmtInt(stats?.epd_refresh_fail_total) },
                    { label: 'Fehlgeschlagene Einschaltvorgänge', content: fmtInt(stats?.epd_poweron_fail_total) },
                    { label: 'Einschaltvorgänge (Panel)', content: fmtInt(stats?.epd_poweron_total) },
                ]}
            />

            <ShareframeInfoCard
                title="Nutzung"
                sections={[
                    { label: 'Aktive Anzeigedauer (kumuliert)', content: fmtDuration(stats?.epd_busy_ms_total) },
                    { label: 'Erste Nutzung', content: fmtUnix(stats?.epd_first_use_at) },
                    { label: 'Letzte Aktualisierung', content: fmtUnix(stats?.epd_last_refresh_at) },
                    { label: 'Dienst-Starts (Display)', content: fmtInt(stats?.app_boot_total) },
                    { label: 'Geräte-Neustarts', content: fmtInt(info?.boot_count) },
                ]}
            />
        </Stack>
    );
};

export default DisplayHealthPage;
