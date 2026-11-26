import React, { useId, useState } from 'react';
import {
    Badge,
    IconButton,
    List,
    ListItem,
    ListItemButton,
    ListItemIcon,
    ListItemText,
    Popover,
    SvgIconProps,
    Tooltip,
    Typography,
} from '@mui/material';
import WifiIcon from '@mui/icons-material/Wifi';
import WifiFindIcon from '@mui/icons-material/WifiFind';
import WifiTetheringIcon from '@mui/icons-material/WifiTethering';
import SignalWifiStatusbarConnectedNoInternet4Icon from '@mui/icons-material/SignalWifiStatusbarConnectedNoInternet4';
import CloudDoneOutlinedIcon from '@mui/icons-material/CloudDoneOutlined';
import CloudSyncOutlinedIcon from '@mui/icons-material/CloudSyncOutlined';
import CloudOffOutlinedIcon from '@mui/icons-material/CloudOffOutlined';
import AddLinkIcon from '@mui/icons-material/AddLink';
import FilterFramesIcon from '@mui/icons-material/FilterFrames';
import UpdateIcon from '@mui/icons-material/Update';
import DeveloperBoardIcon from '@mui/icons-material/DeveloperBoard';
import { useNavigate } from 'react-router';
import { useAppSelector } from '@/store';
import { ConnectionModeState, isServerDown, selectConnectionMode } from '@/store/connectionMode/connectionMode.Slice';
import { selectUpdatesState } from '@/store/updates/updates.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContextValue';
import { getBadgeNumber } from '@/common/utils/badge';
import { getHomeUrl, getUpdatesyUrl } from '@/assets/endpoints/app/appEndpoints';
import { formatDuration } from '@/main/slideshowImages/slideshowImagesUtils';

type StatusColor = 'success' | 'warning' | 'error' | 'info' | 'inherit';

interface StatusRow {
    label: string;
    detail?: string;
    color: StatusColor;
    Icon: React.ComponentType<SvgIconProps>;
}

const networkStatus = ({ mode, ssid, internet, ap_ssid }: ConnectionModeState): StatusRow =>
    mode === 'ap' ? { label: 'Hotspot', detail: `Rahmen stellt den Hotspot „${ap_ssid || 'shareframe-board'}“ bereit`, color: 'info', Icon: WifiTetheringIcon }
        : mode === 'connecting' ? { label: 'Verbinde …', detail: 'Rahmen sucht ein gespeichertes WLAN', color: 'warning', Icon: WifiFindIcon }
            : internet ? { label: 'Online', detail: ssid ? `Mit „${ssid}“ verbunden` : 'Mit dem Internet verbunden', color: 'success', Icon: WifiIcon }
                : { label: 'Kein Internet', detail: ssid ? `Mit „${ssid}“ verbunden, aber ohne Internet` : 'Keine Internetverbindung', color: 'warning', Icon: SignalWifiStatusbarConnectedNoInternet4Icon };

const serverStatus = (connection: ConnectionModeState): StatusRow | null => {
    const { mode, internet, server_connected, registered, server_last_contact_at, server_offline_secs } = connection;
    if (mode !== 'connected' || !internet || server_connected === null) return null;
    if (registered === false) return { label: 'Noch nicht registriert', detail: 'Auf shareframe.de unter „Bilderrahmen hinzufügen“ hinzufügen', color: 'info', Icon: AddLinkIcon };
    if (server_connected) return { label: 'Mit dem Server verbunden', color: 'success', Icon: CloudDoneOutlinedIcon };
    if (!isServerDown(connection)) return { label: 'Verbindet mit dem Server …', color: 'inherit', Icon: CloudSyncOutlinedIcon };
    return {
        label: 'Server nicht erreichbar',
        detail: server_last_contact_at > 0 ? `Letzter Kontakt vor ${formatDuration(server_offline_secs * 1000)}` : 'Noch keine Verbindung',
        color: 'warning',
        Icon: CloudOffOutlinedIcon,
    };
};

const RowContent = ({ label, detail, color, Icon }: StatusRow) => (
    <>
        <ListItemIcon sx={{ minWidth: 40 }}><Icon color={color} /></ListItemIcon>
        <ListItemText primary={label} secondary={detail} />
    </>
);

const SystemStatusMenu = () => {
    const navigate = useNavigate();
    const popoverId = useId();
    const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
    const open = Boolean(anchorEl);

    const connection = useAppSelector(selectConnectionMode);
    const { isConnected, checked } = usePiConnection();
    const boardDown = checked && !isConnected;
    const { latest_release } = useAppSelector(selectUpdatesState);
    const isNewVersion = latest_release?.is_newer ?? false;

    const network = connection.loaded ? networkStatus(connection) : null;
    const server = connection.loaded ? serverStatus(connection) : null;
    const board: StatusRow = isConnected
        ? { label: 'Bilderrahmen erreichbar', color: 'success', Icon: FilterFramesIcon }
        : boardDown
            ? { label: 'Keine Verbindung zum Bilderrahmen', color: 'error', Icon: FilterFramesIcon }
            : { label: 'Bilderrahmen wird geprüft …', color: 'inherit', Icon: FilterFramesIcon };

    const hasWarning = network?.color === 'warning' || server?.color === 'warning';
    const badgeNumber = getBadgeNumber(boardDown, hasWarning, isNewVersion);
    const triggerColor = boardDown ? 'error' : hasWarning ? 'warning' : 'inherit';
    const statusLabel = network ? `System: ${network.label}` : 'System';
    const triggerLabel = badgeNumber > 0 ? `${statusLabel} (${badgeNumber} ${badgeNumber === 1 ? 'Hinweis' : 'Hinweise'})` : statusLabel;

    const handleClose = () => setAnchorEl(null);
    const handleUpdateClick = () => {
        handleClose();
        navigate(getHomeUrl() + getUpdatesyUrl());
    };

    return (
        <>
            <Tooltip title="System">
                <IconButton
                    onClick={(e) => setAnchorEl(e.currentTarget)}
                    aria-label={triggerLabel}
                    aria-haspopup="dialog"
                    aria-expanded={open}
                    aria-controls={open ? popoverId : undefined}
                    sx={{ width: 44, height: 44 }}
                >
                    <Badge badgeContent={badgeNumber > 0 ? badgeNumber : undefined} color="error">
                        <DeveloperBoardIcon color={triggerColor} />
                    </Badge>
                </IconButton>
            </Tooltip>
            <Popover
                id={popoverId}
                open={open}
                anchorEl={anchorEl}
                onClose={handleClose}
                anchorOrigin={{ vertical: 'bottom', horizontal: 'right' }}
                transformOrigin={{ vertical: 'top', horizontal: 'right' }}
                slotProps={{
                    paper: {
                        role: 'dialog',
                        'aria-label': 'System',
                        sx: { width: 360, maxWidth: 'calc(100vw - 32px)' },
                    },
                }}
            >
                <Typography variant="subtitle2" sx={{ px: 2, pt: 1.5 }}>System</Typography>
                <List dense>
                    {network && (
                        <ListItem>
                            <RowContent {...network} />
                        </ListItem>
                    )}
                    {server && (
                        <ListItem>
                            <RowContent {...server} />
                        </ListItem>
                    )}
                    <ListItem>
                        <RowContent {...board} />
                    </ListItem>
                    {isNewVersion && (
                        <ListItem disablePadding>
                            <ListItemButton onClick={handleUpdateClick}>
                                <RowContent
                                    label={`Neue Version (${latest_release?.version}) verfügbar`}
                                    detail="Zu den Updates"
                                    color="error"
                                    Icon={UpdateIcon}
                                />
                            </ListItemButton>
                        </ListItem>
                    )}
                </List>
            </Popover>
        </>
    );
};

export default SystemStatusMenu;
