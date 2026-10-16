import { useState, useEffect } from 'react';
import { Stack, Alert, ToggleButton, ToggleButtonGroup } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectLogsData,
    selectLogsLoading,
    selectLogsError,
    fetchFrameLogs,
} from '@/store/frameLogs/frameLogs.Slice';
import { ServiceType } from '@/types';
import LogsContent from './logsContent';
import LogFilter, { LogFilterParams } from './logFilter';

// Protokolle page: the system log (busybox syslog) or the persistent device event
// log. Per-service application logs live on each service's page under "Verwaltung".
const FrameLogs = () => {
    const dispatch = useAppDispatch();
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const [params, setParams] = useState<LogFilterParams | null>(null);
    const [source, setSource] = useState<ServiceType.SYSTEM | ServiceType.EVENTS>(ServiceType.SYSTEM);

    const loadLogs = () => {
        if (!params) return;
        dispatch(fetchFrameLogs({ service_name: source, ...params }));
    };

    useEffect(() => { loadLogs(); }, [params, source]); // eslint-disable-line react-hooks/exhaustive-deps

    return (
        <Stack spacing={3} sx={{ width: '100%' }}>
            <ToggleButtonGroup
                value={source}
                exclusive
                size="small"
                onChange={(_, value) => value && setSource(value)}
            >
                <ToggleButton value={ServiceType.SYSTEM}>System</ToggleButton>
                <ToggleButton value={ServiceType.EVENTS}>Ereignisse</ToggleButton>
            </ToggleButtonGroup>

            <Alert severity="info">
                {source === ServiceType.SYSTEM
                    ? <>Dies ist das System-Protokoll. Dienst-spezifische Protokolle (Display, WebSocket,
                        Dashboard, Heartbeat) findest du auf der jeweiligen Dienst-Seite unter „Verwaltung".</>
                    : <>Ereignisse bleiben auch nach einem Neustart erhalten: Starts, Abstürze, Updates,
                        WLAN-Wechsel und Befehle, die der Support aus der Ferne ausgelöst hat.</>}
            </Alert>

            <LogFilter onChange={setParams} onRefresh={loadLogs} loading={loading} />

            <LogsContent
                serviceType={source}
                loading={loading}
                error={error}
                logs={logsData && logsData.service === source ? logsData : null}
            />
        </Stack>
    );
};

export default FrameLogs;
