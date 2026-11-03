import React, { useState } from 'react';
import { Button, Chip } from '@mui/material';
import ConfirmDialog from '@/common/components/confirmDialog';
import CircleIcon from '@mui/icons-material/Circle';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import { useAppDispatch, useAppSelector } from '@/store';
import { restartService, selectServiceRestarting } from '@/store/services/services.Slice';
import { ServiceStatus, SERVICE_LABELS } from '@/types';

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
                Neu starten
            </Button>
            <ConfirmDialog
                open={open}
                title={`${SERVICE_LABELS[id] ?? id} neu starten?`}
                confirmLabel="Neu starten"
                onConfirm={() => { dispatch(restartService(id)); setOpen(false); }}
                onCancel={() => setOpen(false)}
            >
                {id === 'dashboard'
                    ? 'Die Einstellungen sind dabei kurz nicht erreichbar.'
                    : 'Der Dienst ist dabei kurz nicht verfügbar.'}
            </ConfirmDialog>
        </>
    );
};
