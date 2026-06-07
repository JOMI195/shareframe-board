import { IAppBarMenuItem } from "@/types";
import FilterFramesIcon from '@mui/icons-material/FilterFrames';
import WifiIcon from '@mui/icons-material/Wifi';
import InfoIcon from '@mui/icons-material/Info';
import UpdateIcon from '@mui/icons-material/Update';
import { getHomeUrl, getGeneralSettingsUrl, getNetworkUrl, getUpdatesyUrl } from "../endpoints/app/appEndpoints";

export const sidebarMenuItems: IAppBarMenuItem[] = [
    {
        name: "Steuerung",
        url: getHomeUrl(),
        icon: FilterFramesIcon
    },
    {
        name: "Netzwerk",
        url: getNetworkUrl(),
        icon: WifiIcon
    },
    {
        name: "Updates",
        url: getUpdatesyUrl(),
        icon: UpdateIcon
    },
    {
        name: "Gerät",
        url: getGeneralSettingsUrl(),
        icon: InfoIcon
    },
];