import { ISidebarSection } from "@/types";
import FilterFramesIcon from '@mui/icons-material/FilterFrames';
import WifiIcon from '@mui/icons-material/Wifi';
import UpdateIcon from '@mui/icons-material/Update';
import InfoIcon from '@mui/icons-material/Info';
import ArticleIcon from '@mui/icons-material/Article';
import MiscellaneousServicesIcon from '@mui/icons-material/MiscellaneousServices';
import TvIcon from '@mui/icons-material/Tv';
import SettingsEthernetIcon from '@mui/icons-material/SettingsEthernet';
import DashboardIcon from '@mui/icons-material/Dashboard';
import MonitorHeartIcon from '@mui/icons-material/MonitorHeart';
import {
    getHomeUrl,
    getGeneralSettingsUrl,
    getLogsUrl,
    getNetworkUrl,
    getUpdatesyUrl,
    getServicesOverviewUrl,
    getServiceDetailUrl,
} from "../endpoints/app/appEndpoints";

// Sidebar is grouped into a user section (always shown) and an admin section
// ("Verwaltung") that is only revealed when advanced mode is enabled.
export const sidebarSections: ISidebarSection[] = [
    {
        section: "Steuerung",
        items: [
            { name: "Steuerung", url: getHomeUrl(), icon: FilterFramesIcon },
            { name: "Netzwerk", url: getNetworkUrl(), icon: WifiIcon },
            { name: "Updates", url: getUpdatesyUrl(), icon: UpdateIcon },
        ],
    },
    {
        section: "Verwaltung",
        advanced: true,
        items: [
            { name: "Dienste", url: getServicesOverviewUrl(), icon: MiscellaneousServicesIcon },
            { name: "Display", url: getServiceDetailUrl("display"), icon: TvIcon },
            { name: "WebSocket", url: getServiceDetailUrl("websocket"), icon: SettingsEthernetIcon },
            { name: "Dashboard", url: getServiceDetailUrl("dashboard"), icon: DashboardIcon },
            { name: "Heartbeat", url: getServiceDetailUrl("heartbeat"), icon: MonitorHeartIcon },
            { name: "System", url: getGeneralSettingsUrl(), icon: InfoIcon },
            { name: "Protokolle", url: getLogsUrl(), icon: ArticleIcon },
        ],
    },
];
