import React, { useEffect, useState } from 'react';
import {
    Box,
    Typography,
    Stack,
    Chip,
    Button,
    Grid,
    Alert,
    useMediaQuery,
    useTheme,
    TextField,
    FormControl,
    InputLabel,
    Select,
    MenuItem,
    Switch,
    FormControlLabel,
} from '@mui/material';
import {
    PlayArrowOutlined,
    StopOutlined,
    DeleteOutlined,
    PhotoLibraryOutlined,
    SkipNextOutlined,
    TimerOutlined,
    SaveOutlined,
    BedtimeOutlined
} from '@mui/icons-material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectSlideshowOperation,
    toggleSlideshowThunk,
    clearDisplayThunk,
    skipImageThunk,
    selectDisplayRefreshInterval,
    updateDisplayImagesLoopInterval,
    selectNightModeEnabled,
    selectNightModeActiveNow,
    selectNightStartHour,
    selectNightEndHour,
    selectNightIntervalMins,
    updateNightMode
} from '@/store/slideshowOperation/slideshowOperation.Slice';
import { selectSlideshowStatus } from '@/store/slideshowStatus/slideshowStatus.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useTimer } from '@/hooks/useTimer';
import { syncSpecificTimer } from '@/store/timers/timers.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const INFO_CARD_HEIGHT = "250px";

const FrameActions: React.FC = () => {
    const dispatch = useAppDispatch();
    const theme = useTheme();

    const { isConnected } = usePiConnection();
    const { isToggling, isClearingDisplay, isSkippingImage, isFetchingInterval, isUpdatingInterval, isFetchingNightMode, isUpdatingNightMode } = useAppSelector(selectSlideshowOperation);
    const { isActive, loopStarted, imageCount, lastCheckedAt, secondsUntilNext } = useAppSelector(selectSlideshowStatus);

    // Interval form state
    const [intervalValue, setIntervalValue] = useState<number>(useAppSelector(selectDisplayRefreshInterval));
    const [intervalUnit, setIntervalUnit] = useState<'minutes' | 'hours'>('minutes');

    // Night mode form state, re-seeded whenever the stored values arrive
    const storedNightEnabled = useAppSelector(selectNightModeEnabled);
    const storedNightStart = useAppSelector(selectNightStartHour);
    const storedNightEnd = useAppSelector(selectNightEndHour);
    const storedNightInterval = useAppSelector(selectNightIntervalMins);
    const isNightActiveNow = useAppSelector(selectNightModeActiveNow);

    const [nightEnabled, setNightEnabled] = useState<boolean>(storedNightEnabled);
    const [nightStart, setNightStart] = useState<number>(storedNightStart);
    const [nightEnd, setNightEnd] = useState<number>(storedNightEnd);
    const [nightInterval, setNightInterval] = useState<number>(storedNightInterval);

    useEffect(() => {
        setNightEnabled(storedNightEnabled);
        setNightStart(storedNightStart);
        setNightEnd(storedNightEnd);
        setNightInterval(storedNightInterval);
    }, [storedNightEnabled, storedNightStart, storedNightEnd, storedNightInterval]);

    // Live countdown to the next image change: seeded from the polled status
    // (every 5s) and ticked locally each second in between.
    const [remaining, setRemaining] = useState<number | null>(secondsUntilNext);
    useEffect(() => {
        setRemaining(secondsUntilNext);
    }, [secondsUntilNext, lastCheckedAt]);
    useEffect(() => {
        const id = setInterval(() => {
            setRemaining((prev) => (prev == null ? prev : Math.max(0, prev - 1)));
        }, 1000);
        return () => clearInterval(id);
    }, []);

    const formatCountdown = (s: number) => `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
    const countdownText =
        !(isConnected && lastCheckedAt !== null) ? 'Unbekannt'
            : !isActive ? 'Pausiert'
                : remaining != null ? formatCountdown(remaining) : '—';

    const {
        isActive: isAppIntialLoadTimerActive,
    } = useTimer({
        id: 'app-initial-load-timer',
        shouldCreate: false
    });

    const {
        isActive: isSlideshowActionsTimerActive,
        formattedTime: slideshowActionsTimerTime
    } = useTimer({
        id: 'slideshow-actions-timer',
    });

    useEffect(() => {
        dispatch(syncSpecificTimer('app-initial-load-timer'));
        dispatch(syncSpecificTimer('slideshow-actions-timer'));

        const syncInterval = setInterval(() => {
            if (isAppIntialLoadTimerActive || isSlideshowActionsTimerActive) {
                dispatch(syncSpecificTimer('app-initial-load-timer'));
                dispatch(syncSpecificTimer('slideshow-actions-timer'));
            }
        }, 1000);

        return () => clearInterval(syncInterval);
    }, [dispatch, isAppIntialLoadTimerActive, isSlideshowActionsTimerActive]);

    const isSmallScreen = useMediaQuery(theme.breakpoints.down("sm"));

    const handleToggleSlideshow = () => {
        dispatch(toggleSlideshowThunk());
    };

    const handleSkipImage = () => {
        dispatch(skipImageThunk());
    };

    const handleClearDisplay = () => {
        dispatch(clearDisplayThunk());
    };

    const handleUpdateInterval = async () => {
        const intervalMins = intervalUnit === 'minutes'
            ? intervalValue
            : intervalValue * 60;
        dispatch(updateDisplayImagesLoopInterval(intervalMins));
    };

    const getMinMaxValues = () => {
        if (intervalUnit === 'minutes') {
            return { min: 5, max: 1440 }; // 5 minutes to 24 hours in minutes
        } else {
            return { min: 5 / 60, max: 24, step: 0.1 }; // 5 minutes to 24 hours
        }
    };

    const { min, max, step = 1 } = getMinMaxValues();

    const NIGHT_MIN_MINS = 5;
    const NIGHT_MAX_MINS = 1440;
    const hours = Array.from({ length: 24 }, (_, h) => h);

    const handleUpdateNightMode = () => {
        dispatch(updateNightMode(nightEnabled, nightStart, nightEnd, nightInterval));
    };

    const isNightFormInvalid = nightStart === nightEnd
        || nightInterval < NIGHT_MIN_MINS
        || nightInterval > NIGHT_MAX_MINS;

    const isLoopLoading = isActive && !loopStarted && isConnected && lastCheckedAt !== null;
    const isTimerRunning = isSlideshowActionsTimerActive || isAppIntialLoadTimerActive;
    const isButtonsDisabled = isTimerRunning || isLoopLoading || isToggling || isClearingDisplay || isSkippingImage || !isConnected || lastCheckedAt === null;

    return (
        <>
            <Grid container spacing={3} sx={{ pb: isSmallScreen ? 7 : 0 }}>
                <Grid item xs={12}>
                    <ShareframeInfoCard
                        title="Nächster Bildwechsel"
                        minHeight="100px"
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Box display="flex" alignItems="center" justifyContent="space-between">
                                            <Box display="flex" alignItems="center" gap={1.5}>
                                                <TimerOutlined color="action" />
                                                <Typography variant="h4" sx={{ fontVariantNumeric: 'tabular-nums' }}>
                                                    {countdownText}
                                                </Typography>
                                            </Box>
                                            <Box display="flex" alignItems="center" gap={1}>
                                                {isNightActiveNow && (
                                                    <Chip
                                                        icon={<BedtimeOutlined />}
                                                        label="Nachtmodus"
                                                        size="small"
                                                        color="primary"
                                                        variant="outlined"
                                                    />
                                                )}
                                                <Chip
                                                    label={
                                                        (isConnected && lastCheckedAt !== null && imageCount != null)
                                                            ? `${imageCount} ${imageCount === 1 ? 'Bild' : 'Bilder'}`
                                                            : '—'
                                                    }
                                                    size="small"
                                                    variant="outlined"
                                                />
                                            </Box>
                                        </Box>
                                    )
                                }
                            }
                        ]}
                    />
                </Grid>

                {!isSlideshowActionsTimerActive && isAppIntialLoadTimerActive && (
                    <Grid item xs={12}>
                        <Alert severity="info" sx={{ display: 'flex', alignItems: 'center' }}>
                            Bitte warte einen Augenblick bis der aktuelle Status der Bildwiedergabe ermittelt wurde
                        </Alert>
                    </Grid>
                )}

                {!isTimerRunning && isLoopLoading && (
                    <Grid item xs={12}>
                        <Alert severity="info" sx={{ display: 'flex', alignItems: 'center' }}>
                            Der Bilderrahmen startet noch. Bitte warte einen Moment.
                        </Alert>
                    </Grid>
                )}

                {isSlideshowActionsTimerActive && (
                    <Grid item xs={12}>
                        <Alert severity="info" sx={{ display: 'flex', alignItems: 'center' }}>
                            Um das Display zu schonen ist die nächste Aktion erst wieder in {slideshowActionsTimerTime}min möglich
                        </Alert>
                    </Grid>
                )}

                <Grid item xs={12} sm={6}>
                    <ShareframeInfoCard
                        title="Bilderwiedergabe"
                        minHeight={INFO_CARD_HEIGHT}
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Box display="flex" alignItems="center">
                                            <PhotoLibraryOutlined sx={{ mr: 1 }} />
                                            <Typography variant="body2">
                                                Die Bildwiedergabe auf dem Bilderrahmen starten oder stoppen.
                                            </Typography>
                                        </Box>
                                    )
                                }
                            },
                            {
                                label: "Status",
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Chip
                                            label={(isConnected && lastCheckedAt !== null) ? isActive ? "Wird ausgeführt" : "Gestoppt" : "Unbekannt"}
                                            color={(isConnected && lastCheckedAt !== null) ? isActive ? "success" : "warning" : "error"}
                                            size="small"
                                        />
                                    )
                                }
                            },
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <Typography variant="body2">
                                                Das Stoppen der Bildwiedergabe leert zusätzlich den Bilderrahmen um das Display vor Schaden zu schützen.
                                            </Typography>

                                        </Stack>
                                    )
                                }
                            }
                        ]}
                        actions={
                            <Button
                                variant="contained"
                                color={isActive ? "error" : "primary"}
                                startIcon={isActive ? <StopOutlined /> : <PlayArrowOutlined />}
                                onClick={handleToggleSlideshow}
                                fullWidth
                                disabled={isButtonsDisabled}
                            >
                                {isActive ? "Stoppen" : "Starten"}
                            </Button>
                        }
                    />
                </Grid>

                <Grid item xs={12} sm={6}>
                    <ShareframeInfoCard
                        title="Bildwechsel-Intervall"
                        minHeight={INFO_CARD_HEIGHT}
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <Box display="flex" alignItems="center">
                                                <TimerOutlined sx={{ mr: 1 }} />
                                                <Typography variant="body2">
                                                    Zeitintervall zwischen den Bildwechseln der Bildwiedergabe konfigurieren.
                                                </Typography>
                                            </Box>
                                            <Box>
                                                <Typography variant="body2">
                                                    Minimum: 5 Minuten
                                                </Typography>
                                                <Typography variant="body2">
                                                    Maximum: 24 Stunden
                                                </Typography>
                                            </Box>
                                            <Box sx={{ display: 'flex', gap: 1 }}>
                                                <TextField
                                                    type="number"
                                                    label="Intervall"
                                                    value={intervalValue}
                                                    onChange={(e) => setIntervalValue(Number(e.target.value))}
                                                    inputProps={{
                                                        min,
                                                        max,
                                                        step
                                                    }}
                                                    sx={{ flex: 1 }}
                                                    disabled={isButtonsDisabled || isUpdatingInterval}
                                                />
                                                <FormControl sx={{ minWidth: 120 }}>
                                                    <InputLabel>Einheit</InputLabel>
                                                    <Select
                                                        value={intervalUnit}
                                                        label="Einheit"
                                                        onChange={(e) => setIntervalUnit(e.target.value as 'minutes' | 'hours')}
                                                        disabled={isButtonsDisabled || isUpdatingInterval}
                                                    >
                                                        <MenuItem value="minutes">Minuten</MenuItem>
                                                        <MenuItem value="hours">Stunden</MenuItem>
                                                    </Select>
                                                </FormControl>
                                            </Box>
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                        actions={
                            <Button
                                variant="contained"
                                startIcon={<SaveOutlined />}
                                onClick={handleUpdateInterval}
                                fullWidth
                                disabled={isButtonsDisabled || isUpdatingInterval || isFetchingInterval || intervalValue < min || intervalValue > max}
                            >
                                Intervall speichern
                            </Button>
                        }
                    />
                </Grid>

                <Grid item xs={12} sm={6}>
                    <ShareframeInfoCard
                        title="Nachtmodus"
                        minHeight={INFO_CARD_HEIGHT}
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <Box display="flex" alignItems="center">
                                                <BedtimeOutlined sx={{ mr: 1 }} />
                                                <Typography variant="body2">
                                                    In den Nachtstunden seltener wechseln, um den Bildschirm zu schonen.
                                                </Typography>
                                            </Box>
                                            <FormControlLabel
                                                control={
                                                    <Switch
                                                        checked={nightEnabled}
                                                        onChange={(e) => setNightEnabled(e.target.checked)}
                                                        disabled={isButtonsDisabled || isUpdatingNightMode}
                                                    />
                                                }
                                                label="Nachtmodus aktivieren"
                                            />
                                            <Box sx={{ display: 'flex', gap: 1 }}>
                                                <FormControl sx={{ flex: 1 }}>
                                                    <InputLabel>Von</InputLabel>
                                                    <Select
                                                        value={nightStart}
                                                        label="Von"
                                                        onChange={(e) => setNightStart(Number(e.target.value))}
                                                        disabled={isButtonsDisabled || isUpdatingNightMode || !nightEnabled}
                                                    >
                                                        {hours.map((h) => (
                                                            <MenuItem key={h} value={h}>{`${h}:00`}</MenuItem>
                                                        ))}
                                                    </Select>
                                                </FormControl>
                                                <FormControl sx={{ flex: 1 }}>
                                                    <InputLabel>Bis</InputLabel>
                                                    <Select
                                                        value={nightEnd}
                                                        label="Bis"
                                                        onChange={(e) => setNightEnd(Number(e.target.value))}
                                                        disabled={isButtonsDisabled || isUpdatingNightMode || !nightEnabled}
                                                    >
                                                        {hours.map((h) => (
                                                            <MenuItem key={h} value={h}>{`${h}:00`}</MenuItem>
                                                        ))}
                                                    </Select>
                                                </FormControl>
                                            </Box>
                                            <TextField
                                                type="number"
                                                label="Intervall in der Nacht (Minuten)"
                                                value={nightInterval}
                                                onChange={(e) => setNightInterval(Number(e.target.value))}
                                                inputProps={{
                                                    min: NIGHT_MIN_MINS,
                                                    max: NIGHT_MAX_MINS,
                                                    step: 1
                                                }}
                                                disabled={isButtonsDisabled || isUpdatingNightMode || !nightEnabled}
                                            />
                                            {nightStart === nightEnd && (
                                                <Typography variant="body2" color="error">
                                                    Start und Ende dürfen nicht gleich sein.
                                                </Typography>
                                            )}
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                        actions={
                            <Button
                                variant="contained"
                                startIcon={<SaveOutlined />}
                                onClick={handleUpdateNightMode}
                                fullWidth
                                disabled={isButtonsDisabled || isUpdatingNightMode || isFetchingNightMode || isNightFormInvalid}
                            >
                                Nachtmodus speichern
                            </Button>
                        }
                    />
                </Grid>

                <Grid item xs={12} sm={6}>
                    <ShareframeInfoCard
                        title="Bild überspringen"
                        minHeight={INFO_CARD_HEIGHT}
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <Box display="flex" alignItems="center">
                                                <SkipNextOutlined sx={{ mr: 1 }} />
                                                <Typography variant="body2">
                                                    Aktuelles Bild in der Bildwiedergabe überspringen.
                                                </Typography>
                                            </Box>
                                            <Typography variant="body2">
                                                Überspringt das aktuell auf dem Bilderrahmen angezeigte Bild und lädt das nächste Bild.
                                            </Typography>
                                            <Typography variant="body2">
                                                Kann nur bei laufender Bildwiedergabe ausgeführt werden.
                                            </Typography>
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                        actions={
                            <Button
                                variant="contained"
                                startIcon={<SkipNextOutlined />}
                                onClick={handleSkipImage}
                                fullWidth
                                disabled={isButtonsDisabled || !isActive}
                            >
                                Bild überspringen
                            </Button>
                        }
                    />
                </Grid>

                <Grid item xs={12} sm={6}>
                    <ShareframeInfoCard
                        title="Bildschirm leeren"
                        minHeight={INFO_CARD_HEIGHT}
                        sections={[
                            {
                                content: {
                                    type: 'reactNode',
                                    value: (
                                        <Stack spacing={2}>
                                            <Box display="flex" alignItems="center">
                                                <DeleteOutlined sx={{ mr: 1 }} />
                                                <Typography variant="body2">
                                                    Die aktuelle Anzeige löschen und einen leeren Bildschirm anzeigen.
                                                </Typography>
                                            </Box>
                                            <Typography variant="body2">
                                                Wenn der Bilderrahmen längere Zeit nicht benutzt wird, MUSS die Anzeige gelöscht werden, da der Bildschirm sonst Schaden nimmt.
                                            </Typography>
                                            <Typography variant="body2">
                                                Um den Bildschirm zu leeren, muss vorher die Bildwiedergabe gestoppt werden.
                                            </Typography>
                                        </Stack>
                                    )
                                }
                            }
                        ]}
                        actions={
                            <Button
                                variant="contained"
                                startIcon={<DeleteOutlined />}
                                onClick={handleClearDisplay}
                                fullWidth
                                disabled={isButtonsDisabled || isActive}
                            >
                                Bildschirm leeren
                            </Button>
                        }
                    />
                </Grid>
            </Grid>
        </>
    );
};

export default FrameActions;