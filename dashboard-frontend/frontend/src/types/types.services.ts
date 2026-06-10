import { ServiceType } from "./types.frameLogs";

// One manageable split service as reported by GET /api/services.
export interface ServiceStatus {
    id: string;                    // websocket | display | dashboard | heartbeat
    label: string;                 // display label (e.g. "Display")
    running: boolean;              // nng health probe (app-level liveness)
    status: string;                // s6 supervision: "up" | "down" | "unknown"
    uptime_seconds: number | null; // time in current s6 state
    pid: number | null;
}

// The services that have a management page (subset of ServiceType: no system).
export const MANAGED_SERVICES: ServiceType[] = [
    ServiceType.DISPLAY,
    ServiceType.WEBSOCKET,
    ServiceType.DASHBOARD,
    ServiceType.HEARTBEAT,
    ServiceType.UPDATE,
];

// German display labels for service ids (shared by sidebar, pages, logs).
export const SERVICE_LABELS: Record<string, string> = {
    websocket: 'WebSocket',
    display: 'Display',
    dashboard: 'Dashboard',
    heartbeat: 'Heartbeat',
    update: 'Updates',
    system: 'System',
};
