import { useState, useEffect } from 'react';
import {
    Typography,
    Card,
    CardContent,
    CardActions,
    Button,
    TextField,
    Grid,
    MenuItem,
} from '@mui/material';
import RefreshIcon from '@mui/icons-material/Refresh';

export interface LogFilterParams {
    since_timestamp: string;
    lines: number;
}

interface LogFilterProps {
    onChange: (params: LogFilterParams) => void;
    onRefresh?: () => void;
    loading?: boolean;
    defaultRange?: string;
    defaultLines?: number;
}

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

const LogFilter: React.FC<LogFilterProps> = ({ onChange, onRefresh, loading, defaultRange = '3h', defaultLines = 1000 }) => {
    const [timeRange, setTimeRange] = useState<string>(defaultRange);
    const [customDate, setCustomDate] = useState<string>(
        new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString().slice(0, 16)
    );
    const [linesParam, setLinesParam] = useState<number>(defaultLines);

    // Backend expects "YYYY-MM-DD HH:MM:SS".
    const getTimestamp = (): string => {
        const date = timeRange === 'custom'
            ? new Date(customDate)
            : new Date(Date.now() - parseInt(timeRange.replace('h', '')) * 60 * 60 * 1000);
        const pad = (num: number) => String(num).padStart(2, '0');
        return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} `
            + `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
    };

    useEffect(() => {
        onChange({ since_timestamp: getTimestamp(), lines: linesParam });
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [timeRange, customDate, linesParam]);

    return (
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
            {onRefresh && (
                <CardActions sx={{ justifyContent: 'flex-end', px: 2, pb: 2 }}>
                    <Button
                        variant="contained"
                        color="primary"
                        startIcon={<RefreshIcon />}
                        onClick={onRefresh}
                        disabled={loading}
                    >
                        Aktualisieren
                    </Button>
                </CardActions>
            )}
        </Card>
    );
};

export default LogFilter;
