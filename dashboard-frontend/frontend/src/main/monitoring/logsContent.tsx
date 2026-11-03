import { ServiceType, SERVICE_LABELS } from "@/types";
import { Box, Button, Card, CardContent, CircularProgress, Divider, Stack, Typography } from "@mui/material";
import RefreshIcon from "@mui/icons-material/Refresh";

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
    onRetry: () => void;
}

const LogsContent: React.FC<LogsContentProps> = ({ loading, error, logs, onRetry }) => {
    if (loading) {
        return (
            <Box
                sx={{
                    display: "flex",
                    justifyContent: "center",
                    p: 3
                }}>
                <CircularProgress />
            </Box>
        );
    }

    if (error) {
        return (
            <Stack spacing={1} sx={{ alignItems: 'flex-start' }}>
                <Typography sx={{ color: "text.secondary" }}>Protokolle konnten nicht geladen werden</Typography>
                <Button variant="outlined" startIcon={<RefreshIcon />} onClick={onRetry}>Erneut versuchen</Button>
            </Stack>
        );
    }

    if (!logs) {
        return (
            <Typography sx={{
                color: "text.secondary"
            }}>Keine Protokolle verfügbar</Typography>
        );
    }

    return (
        <Box>
            <Box sx={{
                mb: 2
            }}>
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
            <Divider sx={{ mb: 2 }} />
            <Card variant="outlined" sx={{ bgcolor: 'background.paper' }}>
                <CardContent role="region" aria-label="Protokolleinträge" tabIndex={0} sx={{
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
                        <Typography sx={{
                            color: "text.secondary"
                        }}>Keine Einträge gefunden</Typography>
                    )}
                </CardContent>
            </Card>
        </Box>
    );
};

export default LogsContent;