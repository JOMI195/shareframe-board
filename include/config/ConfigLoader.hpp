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
        "private_serial_number", &T::privateSerialNumber,
        "public_serial_number", &T::publicSerialNumber,
        "frame_auth_secret_key", &T::frameAuthSecretKey,
        "update_hash_secret_key", &T::updateHashSecretKey
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
        "database_path",   &T::databasePath,
        "database_name",   &T::databaseName,
        "migrations_path", &T::migrationsPath
    );
};

template <>
struct glz::meta<AuthTokenConfig>
{
    using T = AuthTokenConfig;
    static constexpr auto value = glz::object(
        "http_fetch_token_url", &T::httpFetchTokenUrl,
        "http_fetch_token_body_key", &T::httpFetchTokenBodyKey,
        "http_verify_token_url", &T::httpVerifyTokenUrl
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
        "mock_display", &T::mockDisplay,
        "version", &T::version,
        "base_url", &T::baseUrl,
        "log", &T::log,
        "shareframe_application", &T::shareframeApplication,
        "database", &T::database,
        "auth_token", &T::authToken
    );
};

class ConfigLoader
{
public:
    // Converts "dev"/"prod" to Profile; throws std::invalid_argument for unknown values.
    static Profile parseProfile(std::string_view s);

    static AppConfig load(
        Profile profile,
        const std::string& configFilePath = "config.toml",
        const std::string& secretsFilePath = ".env.secrets.toml"
    );

private:
    static void resolvePaths(AppConfig& cfg);
    static void resolveField(std::string& field, const std::filesystem::path& base);
};
