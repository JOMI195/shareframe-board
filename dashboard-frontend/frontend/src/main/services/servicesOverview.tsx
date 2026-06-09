import { useEffect } from 'react';
import { Grid, Stack, Button } from '@mui/material';
import { Link as RouterLink } from 'react-router';
import { useAppDispatch, useAppSelector } from '@/store';
import { fetchServices, selectServices } from '@/store/services/services.Slice';
import { MANAGED_SERVICES, SERVICE_LABELS, ServiceStatus } from '@/types';
import { getServiceDetailUrl } from '@/assets/endpoints/app/appEndpoints';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import { HealthChip, RestartButton, formatUptime } from './serviceUtils';

const POLL_MS = 10000;

const ServicesOverview = () => {
    const dispatch = useAppDispatch();
    const services = useAppSelector(selectServices);

    useEffect(() => {
        dispatch(fetchServices());
        const t = setInterval(() => dispatch(fetchServices()), POLL_MS);
        return () => clearInterval(t);
    }, [dispatch]);

    const byId = (id: string): ServiceStatus | null => services.find((s) => s.id === id) ?? null;

    return (
        <Grid container spacing={3}>
            {MANAGED_SERVICES.map((id) => {
                const svc = byId(id);
                return (
                    <Grid item xs={12} sm={6} md={4} key={id}>
                        <ShareframeInfoCard
                            title={SERVICE_LABELS[id] ?? id}
                            sections={[
                                { label: 'Status', content: { type: 'reactNode', value: <HealthChip service={svc} /> } },
                                {
                                    label: 'Supervision',
                                    content: svc
                                        ? `${svc.status}${svc.uptime_seconds != null ? ` · ${formatUptime(svc.uptime_seconds)}` : ''}`
                                        : '–',
                                },
                            ]}
                            actions={
                                <Stack direction="row" spacing={1}>
                                    <Button component={RouterLink} to={`/${getServiceDetailUrl(id)}`} size="small">
                                        Details
                                    </Button>
                                    <RestartButton id={id} />
                                </Stack>
                            }
                        />
                    </Grid>
                );
            })}
        </Grid>
    );
};

export default ServicesOverview;
