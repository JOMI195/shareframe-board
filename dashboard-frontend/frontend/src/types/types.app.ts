import { SvgIconComponent } from "@mui/icons-material";

export interface IServerResponse {
    success: boolean;
    message: string;
    data: object | null;
}

export interface IAppBarMenuItem {
    name: string;
    url: string;
    icon: SvgIconComponent;
}

export interface ISidebarSection {
    section?: string;   // group header; undefined = ungrouped
    advanced?: boolean; // section only shown when advanced mode is enabled
    items: IAppBarMenuItem[];
}

export interface DialogAction {
    icon: React.ReactNode;
    onClick: () => void;
    label?: string;
    color?: "inherit" | "primary" | "secondary" | "error" | "info" | "success" | "warning";
    disabled?: boolean;
}