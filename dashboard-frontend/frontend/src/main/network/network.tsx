import { useEffect } from 'react';
import {
    Box, Typography, List, ListItem,
    ListItemText, IconButton, CircularProgress,
    Stack
} from '@mui/material';
import DeleteIcon from '@mui/icons-material/Delete';
import WifiIcon from '@mui/icons-material/Wifi';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import { useAppDispatch, useAppSelector } from '@/store';
import {
    fetchNetworkData,
    selectNetworkState
} from '@/store/network/network.Slice';
import {
    openNetworkForgetNetworkDialog
} from '@/store/dialogs/dialogs.Slice';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import { Actions } from './actions/actions';
import Dialogs from './dialogs/dialogs';

const Network = () => {
    const dispatch = useAppDispatch();
    const { isConnected } = usePiConnection();
    const { currentConnection, savedNetworks, loading } = useAppSelector(selectNetworkState);

    const isButtonsDisabled = loading || !isConnected;

    const initiateForgetNetwork = (ssid: string): void => {
        dispatch(openNetworkForgetNetworkDialog(ssid));
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
                />
            </Stack>
            <Actions />
            <Dialogs />
        </>
    );
}

export default Network;