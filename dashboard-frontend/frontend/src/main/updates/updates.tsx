import {
    Box,
    Button,
    Typography,
    Stack,
    CircularProgress,
    LinearProgress,
} from '@mui/material';
import { useEffect } from 'react';
import uuid from 'react-uuid';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectFrameInfoState } from '@/store/frameInfo/frameInfo.Slice';
import { selectUpdatesState, fetchUpdateStatus, fetchLatestRelease } from '@/store/updates/updates.Slice';
import { openUpdatesConfirmUpdateDialog } from '@/store/dialogs/dialogs.Slice';
import { addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import UpdateIcon from '@mui/icons-material/Update';
import ErrorIcon from '@mui/icons-material/Error';
import HourglassTopIcon from '@mui/icons-material/HourglassTop';
import BuildIcon from '@mui/icons-material/Build';
import RefreshIcon from '@mui/icons-material/Refresh';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import ReleaseNotes from '@/common/components/releaseNotes';
import Dialogs from './dialogs/dialogs';
import { pollWhileVisible } from '@/common/utils/poll';
import HistoryIcon from '@mui/icons-material/History';
import { useNavigate } from 'react-router';
import { getHomeUrl, getUpdateHistoryPageUrl } from '@/assets/endpoints/app/appEndpoints';
import { showLoadingWall } from '@/store/loadingWall/loadingWall.Slice';
import { updateFailure } from '@/common/utils/errors/boardErrors';

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
    const navigate = useNavigate();
    const { latest_release, update_status, loading } = useAppSelector(selectUpdatesState);
    const { frameInfo } = useAppSelector(selectFrameInfoState);
    const { isConnected } = usePiConnection();

    const isNewVersion = latest_release?.is_newer ?? false;
    const updateActive = update_status != null
        && ['checking', 'downloading', 'installing', 'awaiting-reboot'].includes(update_status.phase);
    const awaitingConfirm = update_status != null
        && update_status.pending_slot !== ''
        && update_status.pending_slot === update_status.booted_slot;
    const updateBusy = updateActive || awaitingConfirm;

    const handleConfirmUpdate = (): void => {
        dispatch(openUpdatesConfirmUpdateDialog());
    };

    const handleFetchLatestRelease = async (): Promise<void> => {
        const snackbarId = uuid();
        dispatch(addLoadingSnackbar(snackbarId, "Suche nach Updates"));
        await dispatch(fetchLatestRelease());
        dispatch(removeLoadingSnackbar(snackbarId));
    };

    // The page follows download and install; once the frame restarts it is gone until it is back.
    const restarting = update_status?.phase === 'awaiting-reboot' || (updateActive && !isConnected);
    useEffect(() => {
        if (restarting) {
            dispatch(showLoadingWall('Das Update ist installiert. Der Bilderrahmen startet jetzt mit der neuen Version neu. Die Seite lädt von selbst neu, sobald er wieder erreichbar ist.', 10 * 60 * 1000));
        }
    }, [restarting, dispatch]);

    // Poll the installer state; tighter while an update runs.
    useEffect(() => {
        return pollWhileVisible(() => dispatch(fetchUpdateStatus()), updateActive ? 3000 : 15000);
    }, [dispatch, updateActive]);

    return (
        <>
            <Stack spacing={3} sx={{
                width: "100%"
            }}>
                <ShareframeInfoCard
                    title="Übersicht"
                    sections={[
                        {
                            label: "Aktuelle Version",
                            content: frameInfo?.version
                        },
                    ]}
                    actions={
                        <Button startIcon={<HistoryIcon />} onClick={() => navigate(getHomeUrl() + getUpdateHistoryPageUrl())}>
                            Update-Verlauf
                        </Button>
                    }
                />

                {updateActive && (
                    <ShareframeInfoCard
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Box>
                                            <Typography
                                                variant="h6"
                                                gutterBottom
                                                sx={{
                                                    color: "text.secondary",
                                                    display: "flex",
                                                    alignItems: "center"
                                                }}>
                                                <CircularProgress size={21} sx={{ mr: 1 }} />
                                                {phaseLabel(update_status!.phase)}
                                                {update_status!.target_version ? ` (${update_status!.target_version})` : ''}
                                            </Typography>
                                            {update_status!.phase === 'downloading' && update_status!.progress >= 0 ? (
                                                <LinearProgress aria-label="Update-Fortschritt" variant="determinate" value={update_status!.progress} />
                                            ) : (
                                                <LinearProgress aria-label="Update-Fortschritt" />
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
                                        <Typography
                                            variant="h6"
                                            gutterBottom
                                            sx={{
                                                color: "text.secondary",
                                                display: "flex",
                                                alignItems: "center"
                                            }}>
                                            <HourglassTopIcon color="warning" sx={{ mr: 1 }} />Update wird bestätigt
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: "Die neue Version läuft und wird nach der Prüfung automatisch übernommen."
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
                                        <Typography
                                            variant="h6"
                                            gutterBottom
                                            sx={{
                                                color: "text.secondary",
                                                display: "flex",
                                                alignItems: "center"
                                            }}>
                                            <ErrorIcon color="error" sx={{ mr: 1 }} />Update fehlgeschlagen
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={1}>
                                            <Typography variant="body2">{updateFailure(update_status.error)}</Typography>
                                            <Typography variant="caption" sx={{ color: 'text.secondary', wordBreak: 'break-word' }}>
                                                {`Details: ${update_status.error}`}
                                            </Typography>
                                        </Stack>
                                    )
                                }
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
                                        <Typography
                                            variant="h6"
                                            gutterBottom
                                            sx={{
                                                color: "text.secondary",
                                                display: "flex",
                                                alignItems: "center"
                                            }}>
                                            <ErrorIcon color="error" sx={{ mr: 1 }} />Letztes Update zurückgerollt
                                        </Typography>
                                    )
                                }
                            },
                            {
                                content: `Version ${update_status.last_result.to_version} konnte nicht gestartet werden — die vorherige Version ${update_status.last_result.from_version} wurde automatisch wiederhergestellt. Details im Update-Verlauf.`
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
                                                <Typography
                                                    variant="h6"
                                                    gutterBottom
                                                    sx={{
                                                        color: "text.secondary",
                                                        display: "flex",
                                                        alignItems: "center"
                                                    }}>
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
                                                <Typography
                                                    variant="h6"
                                                    gutterBottom
                                                    sx={{
                                                        color: "text.secondary",
                                                        display: "flex",
                                                        alignItems: "center"
                                                    }}>
                                                    <UpdateIcon color='error' sx={{ mr: 1 }} />Neue Version verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        label: "Neue Version",
                                        content: latest_release?.version
                                    },
                                    ...(latest_release?.release_notes?.trim() ? [{
                                        label: "Umfang",
                                        content: {
                                            type: 'reactNode' as const,
                                            value: <ReleaseNotes notes={latest_release.release_notes} />
                                        }
                                    }] : [])
                                ]}
                                actions={
                                    <Button
                                        variant="contained"
                                        color="primary"
                                        startIcon={<BuildIcon />}
                                        onClick={handleConfirmUpdate}
                                        disabled={loading || !isConnected || updateBusy}
                                    >
                                        Jetzt installieren
                                    </Button>
                                }
                            />
                        ) : (
                            <ShareframeInfoCard
                                sections={[
                                    {
                                        content: {
                                            type: 'reactNode',
                                            value: (
                                                <Typography
                                                    variant="h6"
                                                    gutterBottom
                                                    sx={{
                                                        color: "text.secondary",
                                                        display: "flex",
                                                        alignItems: "center"
                                                    }}>
                                                    <CheckCircleIcon color='success' sx={{ mr: 1 }} />Keine Updates verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        content: "Der Bilderrahmen ist auf dem neuesten Stand."
                                    },
                                ]}
                                actions={
                                    <Button
                                        variant="contained"
                                        color="primary"
                                        startIcon={<RefreshIcon />}
                                        onClick={handleFetchLatestRelease}
                                        disabled={loading || !isConnected}
                                    >
                                        Auf Updates prüfen
                                    </Button>
                                }
                            />

                        )
                    )
                }
            </Stack >

            <Dialogs />
        </>
    );
};

export default Updates;