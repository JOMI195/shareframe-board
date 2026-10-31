import { Alert, Button, Stack, Typography } from '@mui/material';
import WifiIcon from '@mui/icons-material/Wifi';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { useAppDispatch, useAppSelector } from '@/store';
import { selectNetworkState } from '@/store/network/network.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { openNetworkConnectDialog } from '@/store/dialogs/dialogs.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import { formatDuration } from '@/main/slideshowImages/slideshowImagesUtils';
import { boardAddress } from '@/common/utils/boardAddress';

const HotspotCard = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { savedNetworks, loading, connectRequestedAt } = useAppSelector(selectNetworkState);
    const { ap_ssid, last_failed_secs_ago, last_failed_at } = useAppSelector(selectConnectionMode);

    const failedSinceRequest = connectRequestedAt !== null && last_failed_at !== null && last_failed_at >= connectRequestedAt;
    const connecting = connectRequestedAt !== null && !failedSinceRequest;

    return (
        <ShareframeInfoCard
            title="Hotspot aktiv"
            minHeight="0px"
            sections={[
                {
                    content: {
                        type: 'reactNode',
                        value: (
                            <Stack spacing={1.5}>
                                <Typography variant="body2" sx={{ color: "text.secondary" }}>
                                    Der Rahmen ist mit keinem WLAN verbunden und stellt gerade den
                                    Hotspot „{ap_ssid || 'shareframe-board'}“ bereit. Füge dein WLAN hinzu
                                    und verbinde den Rahmen anschließend.
                                </Typography>
                                {connecting ? (
                                    <Alert severity="info">
                                        Der Rahmen verbindet sich. Der Hotspot ist dabei aus, diese Seite
                                        verliert die Verbindung. Danach erreichst du ihn in deinem WLAN unter{' '}
                                        <b>{boardAddress(ap_ssid)}</b>. Klappt es nicht, ist der Hotspot nach
                                        etwa einer Minute wieder da.
                                    </Alert>
                                ) : last_failed_secs_ago !== null && (
                                    <Alert severity="warning">
                                        Letzter Verbindungsversuch vor {formatDuration(last_failed_secs_ago * 1000)} fehlgeschlagen:
                                        keines der gespeicherten WLANs war erreichbar. Prüfe Name und Passwort.
                                    </Alert>
                                )}
                            </Stack>
                        )
                    }
                }
            ]}
            actions={
                <Button
                    variant="contained"
                    color="primary"
                    startIcon={<WifiIcon />}
                    onClick={() => dispatch(openNetworkConnectDialog())}
                    disabled={loading || !isConnected || connecting || savedNetworks.length === 0}
                >
                    Jetzt verbinden
                </Button>
            }
        />
    );
};

export default HotspotCard;
