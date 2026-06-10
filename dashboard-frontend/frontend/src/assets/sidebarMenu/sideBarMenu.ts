import { ISidebarSection } from "@/types";
import FilterFramesIcon from '@mui/icons-material/FilterFrames';
import WifiIcon from '@mui/icons-material/Wifi';
import UpdateIcon from '@mui/icons-material/Update';
import InfoIcon from '@mui/icons-material/Info';
import ArticleIcon from '@mui/icons-material/Article';
import HistoryIcon from '@mui/icons-material/History';
import MiscellaneousServicesIcon from '@mui/icons-material/MiscellaneousServices';
import TvIcon from '@mui/icons-material/Tv';
import SettingsEthernetIcon from '@mui/icons-material/SettingsEthernet';
import DashboardIcon from '@mui/icons-material/Dashboard';
import MonitorHeartIcon from '@mui/icons-material/MonitorHeart';
import HealthAndSafetyIcon from '@mui/icons-material/HealthAndSafety';
import {
    getHomeUrl,
    getGeneralSettingsUrl,
    getDisplayHealthUrl,
    getLogsUrl,
    getNetworkUrl,
    getUpdatesyUrl,
    getUpdateHistoryPageUrl,
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
            { name: "Update-Dienst", url: getServiceDetailUrl("update"), icon: UpdateIcon },
            { name: "System", url: getGeneralSettingsUrl(), icon: InfoIcon },
            { name: "Display-Zustand", url: getDisplayHealthUrl(), icon: HealthAndSafetyIcon },
            { name: "Update-Verlauf", url: getUpdateHistoryPageUrl(), icon: HistoryIcon },
            { name: "Protokolle", url: getLogsUrl(), icon: ArticleIcon },
        ],
    },
];
