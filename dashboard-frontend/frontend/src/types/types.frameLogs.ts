// Short, stable service ids — must match the backend log contract
// (SystemHandlers::allowedServiceNames_). No "update": RAUC runs out-of-band
// and writes no spdlog file the dashboard can tail.
export enum ServiceType {
    APPLICATION = 'application',
    DASHBOARD = 'dashboard',
    HEARTBEAT = 'heartbeat',
    SYSTEM = 'system'
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
