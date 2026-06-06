#pragma once
#include "config/AppConfig.hpp"
#include "config/Profile.hpp"
#include <glaze/glaze.hpp>
#include <glaze/toml.hpp>
#include <filesystem>
#include <string>

// glz::meta specializations must be in the header — they are templates
// that must be visible at any translation unit that calls glaze serialization.
template <>
struct glz::meta<SecretsConfig>
{
    using T = SecretsConfig;
    static constexpr auto value = glz::object(
        "public_serial_number", &T::publicSerialNumber,
        "ed25519_private_key", &T::ed25519PrivateKey,
        "update_hash_secret_key", &T::updateHashSecretKey,
        "server_ed25519_public_key", &T::serverEd25519PublicKey
    );
};

template <>
struct glz::meta<LogConfig>
{
    using T = LogConfig;
    static constexpr auto value = glz::object(
        "log_path", &T::logPath
    );
};

template <>
struct glz::meta<ShareframeApplicationConfig>
{
    using T = ShareframeApplicationConfig;
    static constexpr auto value = glz::object(
        "service_name", &T::serviceName,
        "log_file", &T::logFile
    );
};

template <>
struct glz::meta<DatabaseConfig>
{
    using T = DatabaseConfig;
    static constexpr auto value = glz::object(
        "database_path", &T::databasePath,
        "database_name", &T::databaseName,
        "migrations_path", &T::migrationsPath
    );
};

template <>
struct glz::meta<AuthTokenConfig>
{
    using T = AuthTokenConfig;
    static constexpr auto value = glz::object(
        "http_fetch_token_url", &T::httpFetchTokenUrl,
        "http_verify_token_url", &T::httpVerifyTokenUrl
    );
};

template <>
struct glz::meta<WebsocketConfig>
{
    using T = WebsocketConfig;
    static constexpr auto value = glz::object(
        "ws_path", &T::wsPath
    );
};

template <>
struct glz::meta<ImageConfig>
{
    using T = ImageConfig;
    static constexpr auto value = glz::object(
        "image_save_path", &T::imageSavePath
    );
};

struct VersionConfig
{
    std::string version;
};

template <>
struct glz::meta<VersionConfig>
{
    using T = VersionConfig;
    static constexpr auto value = glz::object(
        "version", &T::version
    );
};

template <>
struct glz::meta<HeartbeatApplicationConfig>
{
    using T = HeartbeatApplicationConfig;
    static constexpr auto value = glz::object(
        "service_name", &T::serviceName,
        "log_file", &T::logFile
    );
};

template <>
struct glz::meta<HeartbeatConfig>
{
    using T = HeartbeatConfig;
    static constexpr auto value = glz::object(
        "interval_secs", &T::intervalSecs,
        "http_url", &T::httpUrl
    );
};

template <>
struct glz::meta<ImageCheckConfig>
{
    using T = ImageCheckConfig;
    static constexpr auto value = glz::object(
        "interval_secs", &T::intervalSecs
    );
};

template <>
struct glz::meta<DisplayConfig>
{
    using T = DisplayConfig;
    static constexpr auto value = glz::object(
        "mock_display", &T::mockDisplay,
        "interval_secs", &T::intervalSecs,
        "min_refresh_secs", &T::minRefreshSecs,
        "loading_image_path", &T::loadingImagePath,
        "default_images_path", &T::defaultImagesPath,
        "clear_target_hour", &T::clearTargetHour
    );
};

template <>
struct glz::meta<UpdateConfig>
{
    using T = UpdateConfig;
    static constexpr auto value = glz::object(
        "http_latest_url", &T::httpLatestUrl
    );
};

template <>
struct glz::meta<DashboardApplicationConfig>
{
    using T = DashboardApplicationConfig;
    static constexpr auto value = glz::object(
        "service_name", &T::serviceName,
        "log_file", &T::logFile,
        "socket_path", &T::socketPath,
        "port", &T::port,
        "host", &T::host,
        "http_verify_otp_url", &T::httpVerifyOtpUrl
    );
};

template <>
struct glz::meta<AppConfig>
{
    using T = AppConfig;
    static constexpr auto value = glz::object(
        "base_dir", &T::baseDir,
        "debug", &T::debug,
        "production", &T::production,
        "base_url", &T::baseUrl,
        "log", &T::log,
        "shareframe_application", &T::shareframeApplication,
        "database", &T::database,
        "auth_token", &T::authToken,
        "websocket", &T::websocket,
        "image", &T::image,
        "display", &T::display,
        "dashboard_application", &T::dashboardApplication,
        "heartbeat_application", &T::heartbeatApplication,
        "heartbeat", &T::heartbeat,
        "image_check", &T::imageCheck,
        "update", &T::update
    );
};

class ConfigLoader
{
public:
    // Converts "dev"/"prod" to Profile; throws std::invalid_argument for unknown values.
    static Profile parseProfile(std::string_view s);

    static AppConfig load(Profile profile);

    static AppConfig load(
        Profile profile,
        const std::string& configFilePath,
        const std::string& secretsFilePath,
        const std::string& versionFilePath
    );

private:
    static void resolvePaths(AppConfig& cfg);
    static void resolveField(std::string& field, const std::filesystem::path& base);
    static std::string resolveFile(const char* envVar,
                                   std::initializer_list<std::string_view> candidates);
};
