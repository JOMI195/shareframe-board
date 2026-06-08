import React, { useState, useEffect } from 'react';
import {
    Typography,
    Stack,
    Card,
    CardContent,
    Accordion,
    AccordionSummary,
    AccordionDetails,
    TextField,
    Grid,
    MenuItem,
} from '@mui/material';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    selectLogsData,
    selectLogsLoading,
    selectLogsError,
    clearLogs,
    fetchFrameLogs
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
    { value: 'custom', label: 'Benutzerdefiniert' }
];

const FrameLogs = () => {
    const dispatch = useAppDispatch();
    const logsData = useAppSelector(selectLogsData);
    const loading = useAppSelector(selectLogsLoading);
    const error = useAppSelector(selectLogsError);

    const [expanded, setExpanded] = useState<ServiceType | null>(null);
    const [timeRange, setTimeRange] = useState<string>('3h');
    const [customDate, setCustomDate] = useState<string>(
        new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString().slice(0, 16)
    );
    const [linesParam, setLinesParam] = useState<number>(1000);

    // Calculate timestamp based on selected time range
    const getTimestamp = (): string => {
        let date: Date;

        if (timeRange === 'custom') {
            date = new Date(customDate);
        } else {
            const hours = parseInt(timeRange.replace('h', ''));
            date = new Date(Date.now() - hours * 60 * 60 * 1000);
        }

        // Format: YYYY-MM-DD HH:MM:SS (journalctl-konformes Format)
        const pad = (num: number) => String(num).padStart(2, '0');

        const year = date.getFullYear();
        const month = pad(date.getMonth() + 1);  // Monate sind 0-basiert
        const day = pad(date.getDate());
        const hours = pad(date.getHours());
        const minutes = pad(date.getMinutes());
        const seconds = pad(date.getSeconds());

        return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
    };

    const handleAccordionChange = (serviceType: ServiceType) => (_event: React.SyntheticEvent, isExpanded: boolean) => {
        const newExpanded = isExpanded ? serviceType : null;
        setExpanded(newExpanded);

        if (isExpanded) {
            dispatch(fetchFrameLogs({
                service_name: serviceType,
                since_timestamp: getTimestamp(),
                lines: linesParam
            }));
        } else {
            dispatch(clearLogs());
        }
    };

    const handleTimeRangeChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        setTimeRange(event.target.value);
    };

    const handleCustomDateChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        setCustomDate(event.target.value);
    };

    const handleLinesChange = (event: React.ChangeEvent<HTMLInputElement>) => {
        const value = parseInt(event.target.value);
        if (!isNaN(value) && value > 0) {
            setLinesParam(value);
        }
    };

    const refreshCurrentLogs = () => {
        if (expanded) {
            dispatch(fetchFrameLogs({
                service_name: expanded,
                since_timestamp: getTimestamp(),
                lines: linesParam
            }));
        }
    };

    useEffect(() => {
        refreshCurrentLogs();
    }, [timeRange, linesParam]);

    // Only refresh logs when custom date changes and custom is selected
    useEffect(() => {
        if (timeRange === 'custom') {
            refreshCurrentLogs();
        }
    }, [customDate]);

    return (
        <>
            <Stack spacing={3} sx={{ width: '100%' }}>
                <Card sx={{ mb: 3 }}>
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
                                    onChange={handleTimeRangeChange}
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
                                    onChange={handleLinesChange}
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
                                        onChange={handleCustomDateChange}
                                        InputLabelProps={{ shrink: true }}
                                        helperText="Benutzerdefinierter Startzeitpunkt für Protokolle"
                                    />
                                </Grid>
                            )}
                        </Grid>
                    </CardContent>
                </Card>

                <Stack spacing={1}>
                    <Accordion
                        expanded={expanded === ServiceType.APPLICATION}
                        onChange={handleAccordionChange(ServiceType.APPLICATION)}
                    >
                        <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                            <Typography>Anwendungsprotokolle</Typography>
                        </AccordionSummary>
                        <AccordionDetails>
                            <LogsContent
                                serviceType={ServiceType.APPLICATION}
                                loading={loading}
                                error={error}
                                logs={expanded === ServiceType.APPLICATION ? logsData : null}
                                onRefresh={refreshCurrentLogs}
                            />
                        </AccordionDetails>
                    </Accordion>

                    <Accordion
                        expanded={expanded === ServiceType.DASHBOARD}
                        onChange={handleAccordionChange(ServiceType.DASHBOARD)}
                    >
                        <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                            <Typography>Dashboard-Protokolle</Typography>
                        </AccordionSummary>
                        <AccordionDetails>
                            <LogsContent
                                serviceType={ServiceType.DASHBOARD}
                                loading={loading}
                                error={error}
                                logs={expanded === ServiceType.DASHBOARD ? logsData : null}
                                onRefresh={refreshCurrentLogs}
                            />
                        </AccordionDetails>
                    </Accordion>

                    <Accordion
                        expanded={expanded === ServiceType.HEARTBEAT}
                        onChange={handleAccordionChange(ServiceType.HEARTBEAT)}
                    >
                        <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                            <Typography>Heartbeat-Protokolle</Typography>
                        </AccordionSummary>
                        <AccordionDetails>
                            <LogsContent
                                serviceType={ServiceType.HEARTBEAT}
                                loading={loading}
                                error={error}
                                logs={expanded === ServiceType.HEARTBEAT ? logsData : null}
                                onRefresh={refreshCurrentLogs}
                            />
                        </AccordionDetails>
                    </Accordion>

                    <Accordion
                        expanded={expanded === ServiceType.SYSTEM}
                        onChange={handleAccordionChange(ServiceType.SYSTEM)}
                    >
                        <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                            <Typography>Systemprotokolle</Typography>
                        </AccordionSummary>
                        <AccordionDetails>
                            <LogsContent
                                serviceType={ServiceType.SYSTEM}
                                loading={loading}
                                error={error}
                                logs={expanded === ServiceType.SYSTEM ? logsData : null}
                                onRefresh={refreshCurrentLogs}
                            />
                        </AccordionDetails>
                    </Accordion>
                </Stack>
            </Stack>
        </>
    );
};

export default FrameLogs;