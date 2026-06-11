import { useState, useEffect } from 'react';
import { Stack, Alert } from '@mui/material';
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

// Protokolle page: the system log (busybox syslog). Per-service application logs
// live on each service's page under "Verwaltung".
const FrameLogs = () => {
    const dispatch = useAppDispatch();
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const [params, setParams] = useState<LogFilterParams | null>(null);

    const loadLogs = () => {
        if (!params) return;
        dispatch(fetchFrameLogs({ service_name: ServiceType.SYSTEM, ...params }));
    };

    useEffect(() => { loadLogs(); }, [params]); // eslint-disable-line react-hooks/exhaustive-deps

    return (
        <Stack spacing={3} sx={{ width: '100%' }}>
            <Alert severity="info">
                Dies ist das System-Protokoll. Dienst-spezifische Protokolle (Display, WebSocket,
                Dashboard, Heartbeat) findest du auf der jeweiligen Dienst-Seite unter „Verwaltung".
            </Alert>

            <LogFilter onChange={setParams} onRefresh={loadLogs} loading={loading} />

            <LogsContent
                serviceType={ServiceType.SYSTEM}
                loading={loading}
                error={error}
                logs={logsData && logsData.service === 'system' ? logsData : null}
            />
        </Stack>
    );
};

export default FrameLogs;
