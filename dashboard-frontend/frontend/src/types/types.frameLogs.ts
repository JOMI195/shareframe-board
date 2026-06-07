export enum ServiceType {
    APPLICATION = 'application',
    DASHBOARD = 'dashboard',
    UPDATE = 'update',
    HEARTBEAT = 'heartbeat'
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
