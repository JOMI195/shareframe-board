import { useEffect } from 'react';
import { Stack } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectFrameInfoState, fetchFrameInfos } from '@/store/frameInfo/frameInfo.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const RESOLUTION = "800mm x 480mm";
const SYSTEM_INFO_POLL_MS = 5000;

// --- formatting helpers -----------------------------------------------------
const DASH = '—';

const fmtBytes = (n?: number): string => {
    if (n === undefined || n === null) return DASH;
    const gb = n / 1024 ** 3;
    if (gb >= 1) return `${gb.toFixed(1)} GB`;
    return `${(n / 1024 ** 2).toFixed(0)} MB`;
};

// Used/total + percentage, given a *free* (or available) and total byte count.
const fmtUsage = (free?: number, total?: number): string => {
    if (free === undefined || total === undefined || total === 0) return DASH;
    const used = total - free;
    const pct = Math.round((used / total) * 100);
    return `${fmtBytes(used)} / ${fmtBytes(total)} (${pct}%)`;
};

const fmtUptime = (s?: number): string => {
    if (s === undefined) return DASH;
    const d = Math.floor(s / 86400);
    const h = Math.floor((s % 86400) / 3600);
    const m = Math.floor((s % 3600) / 60);
    const parts: string[] = [];
    if (d) parts.push(`${d}d`);
    if (h || d) parts.push(`${h}h`);
    parts.push(`${m}m`);
    return parts.join(' ');
};

const fmtTemp = (c?: number): string => (c === undefined ? DASH : `${c.toFixed(1)} °C`);
const fmtPct = (p?: number): string => (p === undefined ? DASH : `${p.toFixed(0)} %`);
const fmtLoad = (a?: number, b?: number, c?: number): string =>
    [a, b, c].every((v) => v === undefined) ? DASH : `${a ?? '?'} / ${b ?? '?'} / ${c ?? '?'}`;
const fmtSignal = (d?: number): string => (d === undefined ? DASH : `${d} dBm`);
const fmtTime = (iso?: string): string => {
    if (!iso) return DASH;
    const d = new Date(iso);
    return isNaN(d.getTime()) ? iso : d.toLocaleString('de-DE');
};
const orDash = (v?: string): string => (v && v.length ? v : DASH);

const General = () => {
    const dispatch = useAppDispatch();
    const info = useAppSelector(selectFrameInfoState).frameInfo;

    // Poll live metrics while this page is open (layout fetches once on mount).
    useEffect(() => {
        dispatch(fetchFrameInfos());
        const id = setInterval(() => { dispatch(fetchFrameInfos()); }, SYSTEM_INFO_POLL_MS);
        return () => clearInterval(id);
    }, [dispatch]);

    return (
        <>
            <Stack width={"100%"} spacing={3}>
                <ShareframeInfoCard
                    title="Gerät"
                    sections={[
                        { label: "Seriennummer", content: orDash(info?.serial_number) },
                        { label: "Hostname", content: orDash(info?.hostname) },
                        { label: "Version", content: orDash(info?.version) },
                        { label: "Kernel", content: orDash(info?.kernel) },
                        { label: "Systemzeit", content: fmtTime(info?.time_iso) },
                        { label: "Laufzeit", content: fmtUptime(info?.uptime_seconds) },
                    ]}
                />

                <ShareframeInfoCard
                    title="Netzwerk"
                    sections={[
                        { label: "WLAN", content: orDash(info?.wlan_ssid) },
                        { label: "Signal", content: fmtSignal(info?.wlan_signal_dbm) },
                        { label: "IP (WLAN)", content: orDash(info?.ip_wlan0) },
                        { label: "IP (USB)", content: orDash(info?.ip_usb0) },
                    ]}
                />

                <ShareframeInfoCard
                    title="System"
                    sections={[
                        { label: "CPU-Auslastung", content: fmtPct(info?.cpu_usage_percent) },
                        { label: "CPU-Temperatur", content: fmtTemp(info?.cpu_temp_celsius) },
                        { label: "Load (1/5/15 min)", content: fmtLoad(info?.load_1, info?.load_5, info?.load_15) },
                        { label: "Arbeitsspeicher", content: fmtUsage(info?.ram_available_bytes, info?.ram_total_bytes) },
                        { label: "Speicher (/data)", content: fmtUsage(info?.storage_data_free_bytes, info?.storage_data_total_bytes) },
                    ]}
                />

                <ShareframeInfoCard
                    title="Hardwarespezifikation"
                    sections={[
                        { label: "Display-Größe", content: RESOLUTION },
                    ]}
                />
            </Stack>
        </>
    );
};

export default General;
