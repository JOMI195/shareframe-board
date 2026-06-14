#pragma once
#include <string>

struct SecretsConfig
{
    std::string ed25519PrivateKey;
    // Initial dashboard login password (offline fallback). Only used until the
    // user sets their own (stored hashed in the settings table, which wins).
    std::string dashboardInitialPassword;
};

struct LogConfig
{
    std::string logPath = "logs";
    // General system log (busybox syslogd + klogd) — not a shareframe spdlog
    // file, so it lives outside logPath. Overridable per board/deployment.
    std::string systemLogPath = "/var/log/messages";
};

struct ShareframeApplicationConfig
{
    std::string serviceName = "shareframe.service";
    std::string logFile = "shareframe-application.log";
};

struct DatabaseConfig
{
    std::string databasePath = "db";
    std::string databaseName = "shareframe.db";
    std::string migrationsPath = "migrations";
};

struct AuthTokenConfig
{
    std::string httpFetchTokenUrl = "/api/frames/obtain-frame-token/";
    std::string httpVerifyTokenUrl = "/api/frames/verify-frame-auth-token/";
};

struct WebsocketConfig
{
    std::string wsPath = "ws/frames/";
};

struct ImageConfig
{
    std::string imageSavePath = "images";
};

struct DisplayConfig
{
    bool mockDisplay = false;
    int intervalSecs = 900;
    int minRefreshSecs = 120;
    std::string loadingImagePath = "assets/image/loading";
    std::string defaultImagesPath = "assets/image/default";
    int clearTargetHour = 2;
};

struct UpdateConfig
{
    std::string httpLatestUrl = "/api/frame-updates/latest/";
    int checkIntervalSecs = 21600;
    std::string autoInstallCriticality = "Critical";
};

struct DashboardApplicationConfig
{
    std::string serviceName = "shareframe-dashboard.service";
    std::string logFile = "shareframe-dashboard.log";
    int port = 8080;
    std::string host = "0.0.0.0";
    std::string httpVerifyOtpUrl = "/api/frames/verify-frame-otp/";
};

struct WebsocketApplicationConfig
{
    std::string serviceName = "shareframe-websocket.service";
    std::string logFile = "shareframe-websocket.log";
};

struct DisplayApplicationConfig
{
    std::string serviceName = "shareframe-display.service";
    std::string logFile = "shareframe-display.log";
};

// nng endpoints for cross-process IPC. One source of truth read by every
// service. Defaults live under /tmp (no perms/mkdir needed); the board overlay
// may override to /run/shareframe.
struct IpcConfig
{
    std::string wsRep        = "ipc:///tmp/shareframe-ws.rep.sock";
    std::string wsPub        = "ipc:///tmp/shareframe-ws.pub.sock";
    std::string displayRep   = "ipc:///tmp/shareframe-display.rep.sock";
    std::string dashboardRep = "ipc:///tmp/shareframe-dashboard.rep.sock";
    std::string heartbeatRep = "ipc:///tmp/shareframe-heartbeat.rep.sock";
    std::string updateRep    = "ipc:///tmp/shareframe-update.rep.sock";
};

struct HeartbeatApplicationConfig
{
    std::string serviceName = "shareframe-heartbeat.service";
    std::string logFile = "shareframe-heartbeat.log";
};

struct UpdateApplicationConfig
{
    std::string serviceName = "shareframe-update.service";
    std::string logFile = "shareframe-update.log";
};

struct HeartbeatConfig
{
    int intervalSecs = 60;
    std::string httpUrl = "/api/frames/frame-hearbeat/";
};

struct ExpiryCleanupConfig
{
    int intervalSecs = 900;
};


struct AppConfig
{
    std::string baseDir;
    bool debug = false;
    bool production = false;
    std::string version;
    std::string baseUrl;

    // Runtime-derived (not from TOML): readable frame id = fingerprint of the
    // public key derived from secrets.ed25519PrivateKey. Set in ConfigLoader::load.
    std::string frameId;

    SecretsConfig secrets;
    LogConfig log;
    ShareframeApplicationConfig shareframeApplication;
    DatabaseConfig database;
    AuthTokenConfig authToken;
    WebsocketConfig websocket;
    ImageConfig image;
    DisplayConfig display;
    UpdateConfig update;
    UpdateApplicationConfig updateApplication;
    DashboardApplicationConfig dashboardApplication;
    WebsocketApplicationConfig websocketApplication;
    DisplayApplicationConfig displayApplication;
    HeartbeatApplicationConfig heartbeatApplication;
    HeartbeatConfig heartbeat;
    ExpiryCleanupConfig expiryCleanup;
    IpcConfig ipc;

    [[nodiscard]] std::string httpBaseUrl() const { return (production ? "https://" : "http://") + baseUrl; }
    [[nodiscard]] std::string wsBaseUrl() const { return (production ? "wss://" : "ws://") + baseUrl; }
    [[nodiscard]] std::string wsUrl() const { return wsBaseUrl() + "/" + websocket.wsPath; }
};
