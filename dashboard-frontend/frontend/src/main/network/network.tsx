import { useEffect, useState } from 'react';
import {
    Box, Button, Typography, List, ListItem,
    ListItemText, IconButton, CircularProgress,
    Stack
} from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import WifiIcon from '@mui/icons-material/Wifi';
import AddIcon from '@mui/icons-material/Add';
import RefreshIcon from '@mui/icons-material/Refresh';
import VpnKeyIcon from '@mui/icons-material/VpnKey';
import Visibility from '@mui/icons-material/Visibility';
import VisibilityOff from '@mui/icons-material/VisibilityOff';
import uuid from 'react-uuid';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    fetchNetworkData,
    selectNetworkState
} from '@/store/network/network.Slice';
import {
    openNetworkAddNetworkDialog,
    openNetworkChangeApPasswordDialog,
    openNetworkForgetNetworkDialog
} from '@/store/dialogs/dialogs.Slice';
import { addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';
import { selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import Dialogs from './dialogs/dialogs';

const Network = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { currentConnection, savedNetworks, loading } = useAppSelector(selectNetworkState);
    const { ap_ssid, ap_password } = useAppSelector(selectConnectionMode);
    const [showApPassword, setShowApPassword] = useState(false);

    const isButtonsDisabled = loading || !isConnected;

    const initiateForgetNetwork = (ssid: string): void => {
        dispatch(openNetworkForgetNetworkDialog(ssid));
    };

    const handleAddNetworkDialogOpen = (): void => {
        dispatch(openNetworkAddNetworkDialog());
    };

    const handleRefreshNetworks = async (): Promise<void> => {
        if (!isConnected) return;
        const snackbarId = uuid();
        dispatch(addLoadingSnackbar(snackbarId, "Lade Netzwerke"));
        await dispatch(fetchNetworkData());
        dispatch(removeLoadingSnackbar(snackbarId));
    };

    useEffect(() => {
        dispatch(fetchNetworkData());
    }, [isConnected, dispatch]);

    return (
        <>
            <Stack width={"100%"} spacing={3}>
                <ShareframeInfoCard
                    title="Übersicht (WIFI)"
                    sections={[
                        {
                            label: "Aktuelles Netzwerk",
                            content: {
                                type: 'reactNode',
                                value: (
                                    <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                        <WifiIcon sx={{ mr: 1, color: 'primary.main' }} />
                                        <Typography variant="body2">
                                            {loading ? <CircularProgress size={12} /> : currentConnection}
                                        </Typography>
                                    </Box>
                                )
                            }
                        },
                        {
                            label: "Gespeicherte Netzwerke",
                            content: {
                                type: 'reactNode',
                                value: (
                                    <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                        {loading ? (
                                            <Typography variant="body2">
                                                <CircularProgress size={12} />
                                            </Typography>
                                        ) : (
                                            savedNetworks.length === 0 ? (
                                                <Typography variant="body2" sx={{ textAlign: 'left' }}>
                                                    Keine gespeicherten Netzwerke gefunden (Voreingestellte Netzwerke können nicht verändert werden).
                                                </Typography>
                                            ) : (
                                                <List sx={{ width: '100%' }}>
                                                    {savedNetworks.map((network) => (
                                                        <ListItem
                                                            key={network}
                                                            sx={{
                                                                pr: 16,
                                                                '& .MuiListItemText-root': {
                                                                    overflow: 'hidden',
                                                                    textOverflow: 'ellipsis'
                                                                }
                                                            }}
                                                            secondaryAction={
                                                                <IconButton
                                                                    edge="end"
                                                                    aria-label="delete"
                                                                    onClick={() => initiateForgetNetwork(network)}
                                                                    disabled={isButtonsDisabled}
                                                                >
                                                                    <DeleteIcon />
                                                                </IconButton>
                                                            }
                                                        >
                                                            <ListItemText primary={<Typography variant="body2" noWrap>{network}</Typography>} />
                                                        </ListItem>
                                                    ))}
                                                </List>
                                            )
                                        )}
                                    </Box>
                                )
                            }
                        }
                    ]}
                    actions={
                        <>
                            <Button
                                variant="contained"
                                color="primary"
                                startIcon={<RefreshIcon />}
                                onClick={handleRefreshNetworks}
                                disabled={isButtonsDisabled}
                            >
                                Aktualisieren
                            </Button>
                            <Button
                                variant="contained"
                                color="primary"
                                startIcon={<AddIcon />}
                                onClick={handleAddNetworkDialogOpen}
                                disabled={isButtonsDisabled}
                            >
                                Netzwerk hinzufügen
                            </Button>
                        </>
                    }
                />

                <ShareframeInfoCard
                    title="Access Point"
                    minHeight="0px"
                    sections={[
                        {
                            label: "WLAN-Hotspot (Fallback)",
                            content: {
                                type: 'reactNode',
                                value: (
                                    <Stack spacing={0.5}>
                                        <Typography variant="body2" color="text.secondary">
                                            Findet der Bilderrahmen kein bekanntes WLAN, öffnet er
                                            diesen Hotspot zur Einrichtung.
                                        </Typography>
                                        <Typography variant="body2">
                                            Name: <b>{ap_ssid || '—'}</b>
                                        </Typography>
                                        <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                            <Typography variant="body2">
                                                Passwort: <b>{ap_password
                                                    ? (showApPassword ? ap_password : '••••••••')
                                                    : '—'}</b>
                                            </Typography>
                                            {ap_password && (
                                                <IconButton
                                                    size="small"
                                                    aria-label="AP-Passwort anzeigen"
                                                    onClick={() => setShowApPassword((s) => !s)}
                                                    sx={{ ml: 0.5 }}
                                                >
                                                    {showApPassword
                                                        ? <VisibilityOff fontSize="small" />
                                                        : <Visibility fontSize="small" />}
                                                </IconButton>
                                            )}
                                        </Box>
                                    </Stack>
                                )
                            }
                        }
                    ]}
                    actions={
                        <Button
                            variant="contained"
                            color="primary"
                            startIcon={<VpnKeyIcon />}
                            onClick={() => dispatch(openNetworkChangeApPasswordDialog())}
                            disabled={isButtonsDisabled}
                        >
                            AP-Passwort ändern
                        </Button>
                    }
                />
            </Stack>
            <Dialogs />
        </>
    );
}

export default Network;