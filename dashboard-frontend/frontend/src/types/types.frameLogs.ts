// Short, stable service ids — must match the backend log contract
// (SystemHandlers::allowedServiceNames_). "system" = busybox syslog; the
// others are the split services, each with its own spdlog file; "events" is
// the persistent device event log (boots, crashes, updates, remote commands).
export enum ServiceType {
    WEBSOCKET = 'websocket',
    DISPLAY = 'display',
    DASHBOARD = 'dashboard',
    HEARTBEAT = 'heartbeat',
    UPDATE = 'update',
    SYSTEM = 'system',
    EVENTS = 'events'
}

export interface LogEntry {
    timestamp: string;
    content: string;
}

export interface LogsData {
    service: string;
    period: string;
    timestamp: string;
    log_count: number;
    logs: string[];
    lastFetched?: number;
}

export interface LogsRequestParams {
    service_name: ServiceType;
    since_timestamp?: string;
    lines?: number;
    follow?: boolean;
}
