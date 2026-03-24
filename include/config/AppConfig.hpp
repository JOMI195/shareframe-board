#pragma once
#include <string>

struct SecretsConfig
{
    std::string publicSerialNumber;
    std::string ed25519PrivateKey;
    std::string updateHashSecretKey;
    std::string serverEd25519PublicKey;
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
    std::string httpVerifyTokenUrl;
};

struct WebsocketConfig
{
    std::string wsPath;
};

struct ImageConfig
{
    std::string imageSavePath;
};

struct DisplayConfig
{
    bool mockDisplay = false;
    int intervalSecs = 900;
    int minRefreshSecs = 120;
    std::string loadingImagePath = "static/image/loading";
    std::string defaultImagesPath = "static/image/default";
};

struct UpdateConfig
{
};

struct DashboardConfig
{
};

struct HeartbeatConfig
{
    int intervalSecs;
};

struct ConfigSenderConfig
{
    int intervalSecs;
};

struct ImageCheckConfig
{
    int intervalSecs;
};


struct AppConfig
{
    std::string baseDir;
    bool debug;
    bool production;
    std::string version;
    std::string baseUrl;

    SecretsConfig secrets;
    LogConfig log;
    ShareframeApplicationConfig shareframeApplication;
    DatabaseConfig database;
    AuthTokenConfig authToken;
    WebsocketConfig websocket;
    ImageConfig image;
    DisplayConfig display;
    UpdateConfig update;
    DashboardConfig dashboard;
    HeartbeatConfig heartbeat;
    ConfigSenderConfig configSender;
    ImageCheckConfig imageCheck;

    [[nodiscard]] std::string httpBaseUrl() const { return (production ? "https://" : "http://") + baseUrl; }
    [[nodiscard]] std::string wsBaseUrl() const { return (production ? "wss://" : "ws://") + baseUrl; }
    [[nodiscard]] std::string wsUrl() const { return wsBaseUrl() + "/" + websocket.wsPath; }
};
