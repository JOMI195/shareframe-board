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

export interface DialogAction {
    icon: React.ReactNode;
    onClick: () => void;
    label?: string;
    color?: "inherit" | "primary" | "secondary" | "error" | "info" | "success" | "warning";
    disabled?: boolean;
}