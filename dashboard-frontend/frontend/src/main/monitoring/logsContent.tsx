import { ServiceType, SERVICE_LABELS } from "@/types";
import { Box, Card, CardContent, CircularProgress, Divider, IconButton, Tooltip, Typography } from "@mui/material";
import RefreshIcon from '@mui/icons-material/Refresh';

export interface LogsContentProps {
    serviceType: ServiceType;
    loading: boolean;
    error: string | null;
    logs: {
        service: string;
        period: string;
        timestamp: string;
        log_count: number;
        logs: string[];
    } | null;
    onRefresh: () => void;
}

const LogsContent: React.FC<LogsContentProps> = ({ loading, error, logs, onRefresh }) => {
    if (loading) {
        return (
            <Box display="flex" justifyContent="center" p={3}>
                <CircularProgress />
            </Box>
        );
    }

    if (!logs || error) {
        return <Typography color="text.secondary">Keine Protokolle verfügbar</Typography>;
    }

    return (
        <Box>
            <Box mb={2} display="flex" justifyContent="space-between" alignItems="center">
                <Box>
                    <Typography variant="body2">
                        Dienst: <strong>{SERVICE_LABELS[logs.service] ?? logs.service}</strong>
                    </Typography>
                    <Typography variant="body2">
                        Zeitraum: <strong>{logs.period}</strong>
                    </Typography>
                    <Typography variant="body2">
                        Zeitpunkt: <strong>{new Date(logs.timestamp).toLocaleString('de-DE', {
                            dateStyle: 'medium',
                            timeStyle: 'medium'
                        })}</strong>
                    </Typography>
                    <Typography variant="body2">
                        Anzahl Einträge: <strong>{logs.log_count}</strong>
                    </Typography>
                </Box>
                <Tooltip title="Protokolle aktualisieren">
                    <IconButton
                        onClick={onRefresh}
                        disabled={loading}
                        color="primary"
                    >
                        <RefreshIcon />
                    </IconButton>
                </Tooltip>
            </Box>
            <Divider sx={{ mb: 2 }} />
            <Card variant="outlined" sx={{ bgcolor: 'background.paper' }}>
                <CardContent sx={{
                    p: 2,
                    maxHeight: '400px',
                    overflow: 'auto',
                    fontFamily: 'monospace'
                }}>
                    {logs.logs.length > 0 ? (
                        logs.logs.map((log, index) => (
                            <Box key={index} sx={{
                                py: 0.5,
                                borderBottom: index < logs.logs.length - 1 ? '1px dashed #ddd' : 'none',
                                whiteSpace: 'pre-wrap',
                                wordBreak: 'break-all'
                            }}>
                                {log}
                            </Box>
                        ))
                    ) : (
                        <Typography color="text.secondary">Keine Einträge gefunden</Typography>
                    )}
                </CardContent>
            </Card>
        </Box>
    );
};

export default LogsContent;