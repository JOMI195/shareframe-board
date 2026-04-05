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
    std::string loadingImagePath = "assets/image/loading";
    std::string defaultImagesPath = "assets/image/default";
    int clearTargetHour = 2;
};

struct UpdateConfig
{
    std::string httpLatestUrl = "/api/frame-updates/latest/";
};

struct DashboardApplicationConfig
{
    std::string serviceName = "shareframe-dashboard.service";
    std::string logFile = "shareframe-dashboard.log";
    std::string socketPath = "/tmp/shareframe-ipc.sock";
    int port = 8080;
    std::string host = "0.0.0.0";
    std::string httpVerifyOtpUrl = "/api/frames/verify-frame-otp/";
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
    DashboardApplicationConfig dashboardApplication;
    HeartbeatConfig heartbeat;
    ConfigSenderConfig configSender;
    ImageCheckConfig imageCheck;

    [[nodiscard]] std::string httpBaseUrl() const { return (production ? "https://" : "http://") + baseUrl; }
    [[nodiscard]] std::string wsBaseUrl() const { return (production ? "wss://" : "ws://") + baseUrl; }
    [[nodiscard]] std::string wsUrl() const { return wsBaseUrl() + "/" + websocket.wsPath; }
};
