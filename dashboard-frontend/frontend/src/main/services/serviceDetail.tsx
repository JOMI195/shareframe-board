import React, { useEffect, useState } from 'react';
import { Stack, Box, Typography } from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchServices, selectService } from '@/store/services/services.Slice';
import {
    fetchFrameLogs, selectLogsData, selectLogsLoading, selectLogsError, clearLogs,
} from '@/store/frameLogs/frameLogs.Slice';
import { ServiceType, SERVICE_LABELS } from '@/types';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import LogsContent from '@/main/monitoring/logsContent';
import LogFilter, { LogFilterParams } from '@/main/monitoring/logFilter';
import { HealthChip, RestartButton, formatUptime } from './serviceUtils';

const POLL_MS = 10000;

const ServiceDetail: React.FC<{ serviceId: ServiceType }> = ({ serviceId }) => {
    const dispatch = useAppDispatch();
    const svc = useAppSelector(selectService(serviceId));
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const [params, setParams] = useState<LogFilterParams | null>(null);

    const loadLogs = () => {
        if (!params) return;
        dispatch(fetchFrameLogs({ service_name: serviceId, ...params }));
    };

    useEffect(() => {
        dispatch(fetchServices());
        loadLogs();
        const t = setInterval(() => dispatch(fetchServices()), POLL_MS);
        return () => {
            clearInterval(t);
            dispatch(clearLogs());
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [serviceId, params]);

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
                <Box mb={3}>
                    <LogFilter onChange={setParams} defaultLines={500} />
                </Box>
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
