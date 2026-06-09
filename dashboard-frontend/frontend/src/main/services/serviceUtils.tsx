import React, { useState } from 'react';
import {
    Button, Chip, Dialog, DialogActions, DialogContent, DialogTitle, Typography,
} from '@mui/material';
import CircleIcon from '@mui/icons-material/Circle';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import { useAppDispatch, useAppSelector } from '@/store';
import { restartService, selectServiceRestarting } from '@/store/services/services.Slice';
import { ServiceStatus, SERVICE_LABELS } from '@/types';

export const formatUptime = (s: number | null): string => {
    if (s == null) return '–';
    if (s < 60) return `${s}s`;
    const m = Math.floor(s / 60), h = Math.floor(m / 60), d = Math.floor(h / 24);
    if (d > 0) return `${d}d ${h % 24}h`;
    if (h > 0) return `${h}h ${m % 60}m`;
    return `${m}m`;
};

// Liveness chip from the nng health probe.
export const HealthChip: React.FC<{ service: ServiceStatus | null }> = ({ service }) => {
    if (!service) return <Chip size="small" label="Unbekannt" color="default" />;
    const ok = service.running;
    return (
        <Chip
            size="small"
            icon={<CircleIcon sx={{ fontSize: 12 }} />}
            label={ok ? 'Aktiv' : 'Offline'}
            color={ok ? 'success' : 'error'}
            variant={ok ? 'filled' : 'outlined'}
        />
    );
};

// Restart button with a confirm dialog; warns specially for the dashboard
// (restarting it briefly drops the very connection serving the UI).
export const RestartButton: React.FC<{ id: string; size?: 'small' | 'medium' }> = ({ id, size = 'small' }) => {
    const dispatch = useAppDispatch();
    const restarting = useAppSelector(selectServiceRestarting);
    const [open, setOpen] = useState(false);

    return (
        <>
            <Button
                size={size}
                color="warning"
                startIcon={<RestartAltIcon />}
                disabled={restarting === id}
                onClick={() => setOpen(true)}
            >
                Neustart
            </Button>
            <Dialog open={open} onClose={() => setOpen(false)}>
                <DialogTitle>Dienst neu starten?</DialogTitle>
                <DialogContent>
                    <Typography>
                        {(SERVICE_LABELS[id] ?? id)} wird neu gestartet.
                        {id === 'dashboard' && ' Das Dashboard ist dabei kurz nicht erreichbar.'}
                    </Typography>
                </DialogContent>
                <DialogActions>
                    <Button onClick={() => setOpen(false)}>Abbrechen</Button>
                    <Button color="warning" onClick={() => { dispatch(restartService(id)); setOpen(false); }}>
                        Neustart
                    </Button>
                </DialogActions>
            </Dialog>
        </>
    );
};
