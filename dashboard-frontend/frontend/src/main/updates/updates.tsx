import {
    Box,
    Typography,
    Stack,
    CircularProgress,
    LinearProgress,
} from '@mui/material';
import { useEffect } from 'react';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectFrameInfoState } from '@/store/frameInfo/frameInfo.Slice';
import { selectUpdatesState, fetchUpdateStatus } from '@/store/updates/updates.Slice';
import { isVersionNewer } from '@/common/utils/version';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import UpdateIcon from '@mui/icons-material/Update';
import ErrorIcon from '@mui/icons-material/Error';
import HourglassTopIcon from '@mui/icons-material/HourglassTop';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import Dialogs from './dialogs/dialogs';
import { Actions } from './actions/actions';

const phaseLabel = (phase: string): string => {
    switch (phase) {
        case 'checking': return 'Prüfe Update';
        case 'downloading': return 'Lade Update herunter';
        case 'installing': return 'Installiere Update';
        case 'awaiting-reboot': return 'Neustart wird vorbereitet';
        default: return phase;
    }
};

const Updates = () => {
    const dispatch = useAppDispatch();
    const { latest_release, update_status, loading } = useAppSelector(selectUpdatesState);
    const { frameInfo } = useAppSelector(selectFrameInfoState);

    const isNewVersion = (latest_release && frameInfo && isVersionNewer(latest_release.version, frameInfo.version)) ?? false;
    const updateActive = update_status != null
        && ['checking', 'downloading', 'installing', 'awaiting-reboot'].includes(update_status.phase);
    const awaitingConfirm = update_status != null
        && update_status.pending_slot !== ''
        && update_status.pending_slot === update_status.booted_slot;

    // Poll the installer state; tighter while an update runs.
    useEffect(() => {
        dispatch(fetchUpdateStatus());
        const interval = setInterval(() => dispatch(fetchUpdateStatus()), updateActive ? 3000 : 15000);
        return () => clearInterval(interval);
    }, [dispatch, updateActive]);

    return (
        <>
            <Stack width={"100%"} spacing={3}>
                <ShareframeInfoCard
                    title="Übersicht"
                    sections={[
                        {
                            label: "Aktuelle Version",
                            content: frameInfo?.version
                        },
                    ]}
                />

                {updateActive && (
                    <ShareframeInfoCard
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Box>
                                            <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                <CircularProgress size={21} sx={{ mr: 1 }} />
                                                {phaseLabel(update_status!.phase)}
                                                {update_status!.target_version ? ` (${update_status!.target_version})` : ''}
                                            </Typography>
                                            {update_status!.phase === 'downloading' && update_status!.progress >= 0 ? (
                                                <LinearProgress variant="determinate" value={update_status!.progress} />
                                            ) : (
                                                <LinearProgress />
                                            )}
                                        </Box>
                                    )
                                }
                            }
                        ]}
                    />
                )}

                {!updateActive && awaitingConfirm && (
                    <ShareframeInfoCard
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                            <HourglassTopIcon color="warning" sx={{ mr: 1 }} />Update wird bestätigt
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: "Die neue Version läuft und wird nach erfolgreicher Prüfung automatisch übernommen."
                            }
                        ]}
                    />
                )}

                {!updateActive && update_status?.phase === 'failed' && (
                    <ShareframeInfoCard
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                            <ErrorIcon color="error" sx={{ mr: 1 }} />Update fehlgeschlagen
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: update_status.error
                            }
                        ]}
                    />
                )}

                {!updateActive && update_status?.last_result?.result === 'rolled-back' && (
                    <ShareframeInfoCard
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                            <ErrorIcon color="error" sx={{ mr: 1 }} />Letztes Update zurückgerollt
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: `Version ${update_status.last_result.to_version} konnte nicht gestartet werden — die vorherige Version ${update_status.last_result.from_version} wurde automatisch wiederhergestellt. Details im Update-Verlauf (Verwaltung).`
                            }
                        ]}
                    />
                )}

                {
                    loading ? (
                        <ShareframeInfoCard
                            sections={[
                                {
                                    content: {
                                        type: 'reactNode',
                                        value: (
                                            <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <CircularProgress size={21} sx={{ mr: 1 }} />Suche nach Updates
                                                </Typography>
                                            </Box>
                                        )
                                    },
                                }
                            ]}
                        />
                    ) : (
                        isNewVersion ? (
                            <ShareframeInfoCard
                                sections={[
                                    {
                                        content: {
                                            type: 'reactNode',
                                            value: (
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <UpdateIcon color='error' sx={{ mr: 1 }} />Neue Version verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        label: "Neue Version",
                                        content: latest_release?.version
                                    },
                                    {
                                        label: "Umfang",
                                        content: latest_release?.release_notes
                                    }
                                ]}
                            />
                        ) : (
                            <ShareframeInfoCard
                                sections={[
                                    {
                                        content: {
                                            type: 'reactNode',
                                            value: (
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <CheckCircleIcon color='success' sx={{ mr: 1 }} />Keine Updates verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        content: "Bilderrahmen ist auf dem neusten Stand!"
                                    },
                                ]}
                            />

                        )
                    )
                }
            </Stack >

            <Actions />
            <Dialogs />
        </>
    );
};

export default Updates;