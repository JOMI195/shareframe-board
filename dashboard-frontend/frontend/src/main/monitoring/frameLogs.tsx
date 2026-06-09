import { useState, useEffect } from 'react';
import {
    Typography,
    Stack,
    Card,
    CardContent,
    TextField,
    Grid,
    MenuItem,
    Alert,
} from '@mui/material';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectLogsData,
    selectLogsLoading,
    selectLogsError,
    fetchFrameLogs,
} from '@/store/frameLogs/frameLogs.Slice';
import { ServiceType } from '@/types';
import LogsContent from './logsContent';

const timeRangeOptions = [
    { value: '1h', label: 'Letzte Stunde' },
    { value: '3h', label: 'Letzte 3 Stunden' },
    { value: '6h', label: 'Letzte 6 Stunden' },
    { value: '12h', label: 'Letzte 12 Stunden' },
    { value: '24h', label: 'Letzter Tag' },
    { value: '48h', label: 'Letzte 2 Tage' },
    { value: '72h', label: 'Letzte 3 Tage' },
    { value: '168h', label: 'Letzte Woche' },
    { value: 'custom', label: 'Benutzerdefiniert' },
];

// Protokolle page: the system log (busybox syslog). Per-service application logs
// live on each service's page under "Verwaltung".
const FrameLogs = () => {
    const dispatch = useAppDispatch();
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const [timeRange, setTimeRange] = useState<string>('3h');
    const [customDate, setCustomDate] = useState<string>(
        new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString().slice(0, 16)
    );
    const [linesParam, setLinesParam] = useState<number>(1000);

    // Backend expects "YYYY-MM-DD HH:MM:SS".
    const getTimestamp = (): string => {
        const date = timeRange === 'custom'
            ? new Date(customDate)
            : new Date(Date.now() - parseInt(timeRange.replace('h', '')) * 60 * 60 * 1000);
        const pad = (num: number) => String(num).padStart(2, '0');
        return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} `
            + `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
    };

    const loadLogs = () => {
        dispatch(fetchFrameLogs({
            service_name: ServiceType.SYSTEM,
            since_timestamp: getTimestamp(),
            lines: linesParam,
        }));
    };

    useEffect(() => { loadLogs(); }, [timeRange, linesParam]);
    useEffect(() => { if (timeRange === 'custom') loadLogs(); }, [customDate]);

    return (
        <Stack spacing={3} sx={{ width: '100%' }}>
            <Alert severity="info">
                Dies ist das System-Protokoll. Dienst-spezifische Protokolle (Display, WebSocket,
                Dashboard, Heartbeat) findest du auf der jeweiligen Dienst-Seite unter „Verwaltung".
            </Alert>

            <Card>
                <CardContent>
                    <Typography variant="h6" color="text.secondary" gutterBottom>
                        Filter
                    </Typography>
                    <Grid container spacing={2}>
                        <Grid item xs={12} sm={6}>
                            <TextField
                                select
                                label="Zeitraum"
                                fullWidth
                                value={timeRange}
                                onChange={(e) => setTimeRange(e.target.value)}
                                helperText="Zeitraum der abzurufenden Protokolle"
                            >
                                {timeRangeOptions.map((option) => (
                                    <MenuItem key={option.value} value={option.value}>
                                        {option.label}
                                    </MenuItem>
                                ))}
                            </TextField>
                        </Grid>
                        <Grid item xs={12} sm={6}>
                            <TextField
                                label="Anzahl Zeilen"
                                type="number"
                                fullWidth
                                value={linesParam}
                                onChange={(e) => {
                                    const value = parseInt(e.target.value);
                                    if (!isNaN(value) && value > 0) setLinesParam(value);
                                }}
                                InputProps={{ inputProps: { min: 1, max: 5000 } }}
                                helperText="Maximale Anzahl anzuzeigender Protokollzeilen"
                            />
                        </Grid>
                        {timeRange === 'custom' && (
                            <Grid item xs={12}>
                                <TextField
                                    label="Datum und Uhrzeit"
                                    type="datetime-local"
                                    fullWidth
                                    value={customDate}
                                    onChange={(e) => setCustomDate(e.target.value)}
                                    InputLabelProps={{ shrink: true }}
                                    helperText="Benutzerdefinierter Startzeitpunkt für Protokolle"
                                />
                            </Grid>
                        )}
                    </Grid>
                </CardContent>
            </Card>

            <LogsContent
                serviceType={ServiceType.SYSTEM}
                loading={loading}
                error={error}
                logs={logsData && logsData.service === 'system' ? logsData : null}
                onRefresh={loadLogs}
            />
        </Stack>
    );
};

export default FrameLogs;
