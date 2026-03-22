#pragma once
#include <string>

struct SecretsConfig
{
    std::string privateSerialNumber;
    std::string publicSerialNumber;
    std::string frameAuthSecretKey;
    std::string updateHashSecretKey;
};

struct LogConfig
{
    std::string logPath;
};

struct ShareframeApplicationConfig
{
    std::string serviceName;
    std::string logFile;
};

struct DatabaseConfig
{
    std::string databasePath;
    std::string databaseName;
    std::string migrationsPath;
};

struct AuthTokenConfig
{
    std::string httpFetchTokenUrl;
    std::string httpFetchTokenBodyKey;
    std::string httpVerifyTokenUrl;
};

struct WebsocketConfig
{
    std::string wsPath;
};

struct DisplayConfig
{
};

struct UpdateConfig
{
};

struct DashboardConfig
{
};

struct HeartbeatConfig
{
    int intervalSecs = 300;
};

struct ConfigSenderConfig
{
    int intervalSecs = 900;
};


struct AppConfig
{
    std::string baseDir;
    bool debug;
    bool production;
    bool mockDisplay;
    std::string version;
    std::string baseUrl;

    SecretsConfig secrets;
    LogConfig log;
    ShareframeApplicationConfig shareframeApplication;
    DatabaseConfig database;
    AuthTokenConfig authToken;
    WebsocketConfig websocket;
    DisplayConfig display;
    UpdateConfig update;
    DashboardConfig dashboard;
    HeartbeatConfig heartbeat;
    ConfigSenderConfig configSender;

    [[nodiscard]] std::string httpBaseUrl() const { return (production ? "https://" : "http://") + baseUrl; }
    [[nodiscard]] std::string wsBaseUrl() const { return (production ? "wss://" : "ws://") + baseUrl; }
    [[nodiscard]] std::string wsUrl() const { return wsBaseUrl() + "/" + websocket.wsPath; }
};
