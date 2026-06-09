import React, { useEffect } from 'react';
import { Stack, Box, Typography } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchServices, selectService } from '@/store/services/services.Slice';
import {
    fetchFrameLogs, selectLogsData, selectLogsLoading, selectLogsError, clearLogs,
} from '@/store/frameLogs/frameLogs.Slice';
import { ServiceType, SERVICE_LABELS } from '@/types';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import LogsContent from '@/main/monitoring/logsContent';
import { HealthChip, RestartButton, formatUptime } from './serviceUtils';

const POLL_MS = 10000;
const DEFAULT_HOURS = 3;
const DEFAULT_LINES = 500;

// Backend expects "YYYY-MM-DD HH:MM:SS".
const sinceTimestamp = (): string => {
    const d = new Date(Date.now() - DEFAULT_HOURS * 60 * 60 * 1000);
    const pad = (n: number) => String(n).padStart(2, '0');
    return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
};

const ServiceDetail: React.FC<{ serviceId: ServiceType }> = ({ serviceId }) => {
    const dispatch = useAppDispatch();
    const svc = useAppSelector(selectService(serviceId));
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const loadLogs = () =>
        dispatch(fetchFrameLogs({ service_name: serviceId, since_timestamp: sinceTimestamp(), lines: DEFAULT_LINES }));

    useEffect(() => {
        dispatch(fetchServices());
        loadLogs();
        const t = setInterval(() => dispatch(fetchServices()), POLL_MS);
        return () => {
            clearInterval(t);
            dispatch(clearLogs());
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [serviceId]);

    return (
        <Stack spacing={3} sx={{ width: '100%' }}>
            <ShareframeInfoCard
                title={SERVICE_LABELS[serviceId] ?? serviceId}
                sections={[
                    { label: 'Status', content: { type: 'reactNode', value: <HealthChip service={svc} /> } },
                    { label: 'Supervision', content: svc ? svc.status : '–' },
                    { label: 'Laufzeit', content: svc ? formatUptime(svc.uptime_seconds) : '–' },
                    { label: 'PID', content: svc?.pid != null ? String(svc.pid) : '–' },
                ]}
                actions={<RestartButton id={serviceId} size="medium" />}
            />

            <Box>
                <Typography variant="h6" color="text.secondary" gutterBottom>
                    Protokolle
                </Typography>
                <LogsContent
                    serviceType={serviceId}
                    loading={loading}
                    error={error}
                    logs={logsData && logsData.service === serviceId ? logsData : null}
                    onRefresh={loadLogs}
                />
            </Box>
        </Stack>
    );
};

export default ServiceDetail;
