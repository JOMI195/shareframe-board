#include "config/ConfigLoader.hpp"
#include "auth/FrameIdentity.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

Profile ConfigLoader::parseProfile(std::string_view s)
{
    if (s == "dev")        return Profile::Dev;
    if (s == "prod")       return Profile::Prod;
    if (s == "prod.local") return Profile::ProdLocal;
    throw std::invalid_argument(std::string("Unknown profile '") + std::string(s) + "'. Valid values: dev, prod, prod.local");
}

static glz::error_ctx readTomlFile(auto& val, const std::string& path) {
    std::string buf;
    if (std::ifstream f{path}; f)
        buf.assign(std::istreambuf_iterator<char>(f), {});
    else
        throw std::runtime_error("Cannot open config file: " + path);
    return glz::read<glz::opts{.format = glz::TOML, .error_on_unknown_keys = false}>(val, buf);
}

std::string ConfigLoader::resolveFile(const char* envVar,
                                      std::initializer_list<std::string_view> candidates)
{
    if (const char* override = std::getenv(envVar); override && *override)
        return override;
    for (auto p : candidates)
        if (std::filesystem::exists(p))
            return std::string(p);
    return std::string(*candidates.begin());
}

AppConfig ConfigLoader::load(Profile profile)
{
    return load(profile,
        resolveFile("SHAREFRAME_CONFIG_FILE",
                    {"/etc/shareframe/config.toml", "config.toml"}),
        resolveFile("SHAREFRAME_SECRETS_FILE",
                    {"/data/shareframe/.env.secrets.toml",
                     "/etc/shareframe/.env.secrets.toml",
                     ".env.secrets.toml"}),
        resolveFile("SHAREFRAME_VERSION_FILE",
                    {"/etc/shareframe/VERSION.toml", "VERSION.toml"}));
}

AppConfig ConfigLoader::load(
    Profile profile,
    const std::string& configFilePath,
    const std::string& secretsFilePath,
    const std::string& versionFilePath
) {
    AppConfig cfg;

    // Pass 1: base config
    auto err = readTomlFile(cfg, configFilePath);
    if (err)
        throw std::runtime_error(
            std::string("Failed to load config '") + configFilePath
            + "': " + glz::format_error(err, ""));

    // Pass 2: profile overlay — only keys present in the file are updated
    namespace fs = std::filesystem;
    const fs::path basePath(configFilePath);
    const std::string profileFile =
        (basePath.parent_path() / (basePath.stem().string() + "." + std::string(profileName(profile)) + ".toml")).string();

    if (fs::exists(profileFile)) {
        auto perr = readTomlFile(cfg, profileFile);
        if (perr)
            throw std::runtime_error(
                std::string("Failed to load profile config '") + profileFile
                + "': " + glz::format_error(perr, ""));
    } else {
        std::cerr << "[config] Profile file not found, using base config: " << profileFile << '\n';
    }

    // Secrets — lenient on unknown keys so older firmware keeps booting after
    // a newer key is provisioned on /data (A/B rollback safety).
    SecretsConfig secrets;
    auto serr = readTomlFile(secrets, secretsFilePath);
    if (serr)
        throw std::runtime_error(
            std::string("Failed to load secrets '") + secretsFilePath
            + "': " + glz::format_error(serr, ""));
    cfg.secrets = secrets;

    // Derive the readable frame id from the private seed (single source of truth)
    if (!cfg.secrets.ed25519PrivateKey.empty())
        cfg.frameId = FrameIdentity::fingerprint(cfg.secrets.ed25519PrivateKey);

    if (std::filesystem::exists(versionFilePath)) {
        VersionConfig ver;
        auto verr = readTomlFile(ver, versionFilePath);
        if (verr)
            throw std::runtime_error(
                std::string("Failed to load version '") + versionFilePath
                + "': " + glz::format_error(verr, ""));
        cfg.version = ver.version;
    } else {
        cfg.version = "dev";
    }

    resolvePaths(cfg);
    validate(cfg);
    return cfg;
}

void ConfigLoader::validate(const AppConfig& cfg)
{
    std::vector<std::string> errs;
    const auto req = [&](bool ok, const std::string& msg) { if (!ok) errs.push_back(msg); };

    // Deployment-specific, no safe default — must be set by a profile/secrets.
    req(!cfg.baseUrl.empty(), "base_url is empty (set it in the profile config)");

    // Have code defaults; a profile could still blank/zero them — guard anyway.
    req(!cfg.database.databasePath.empty(), "database.database_path is empty");
    req(!cfg.database.databaseName.empty(), "database.database_name is empty");
    req(!cfg.log.logPath.empty(), "log.log_path is empty");
    req(!cfg.image.imageSavePath.empty(), "image.image_save_path is empty");
    req(!cfg.websocket.wsPath.empty(), "websocket.ws_path is empty");

    req(cfg.heartbeat.intervalSecs > 0, "heartbeat.interval_secs must be > 0");
    req(cfg.expiryCleanup.intervalSecs > 0, "expiry_cleanup.interval_secs must be > 0");
    req(cfg.display.intervalSecs > 0, "display.interval_secs must be > 0");
    req(cfg.display.minRefreshSecs > 0, "display.min_refresh_secs must be > 0");
    req(cfg.display.clearTargetHour >= 0 && cfg.display.clearTargetHour <= 23,
        "display.clear_target_hour must be in 0..23");
    req(cfg.dashboardApplication.port > 0 && cfg.dashboardApplication.port <= 65535,
        "dashboard_application.port must be in 1..65535");

    // IPC endpoints — every service depends on these resolving to a real address.
    req(!cfg.ipc.wsRep.empty(), "ipc.ws_rep is empty");
    req(!cfg.ipc.wsPub.empty(), "ipc.ws_pub is empty");
    req(!cfg.ipc.displayRep.empty(), "ipc.display_rep is empty");
    req(!cfg.ipc.dashboardRep.empty(), "ipc.dashboard_rep is empty");
    req(!cfg.ipc.heartbeatRep.empty(), "ipc.heartbeat_rep is empty");

    if (!errs.empty())
    {
        std::string joined = "Invalid configuration:";
        for (const auto& e : errs)
            joined += "\n  - " + e;
        throw std::runtime_error(joined);
    }
}

void ConfigLoader::resolveField(std::string& field, const std::filesystem::path& base) {
    std::filesystem::path p(field);
    if (!p.is_absolute())
        field = (base / p).lexically_normal().string();
}

void ConfigLoader::resolvePaths(AppConfig& cfg) {
    const std::filesystem::path base = std::filesystem::path(cfg.baseDir).lexically_normal();
    resolveField(cfg.log.logPath,          base);
    resolveField(cfg.database.databasePath,   base);
    resolveField(cfg.database.migrationsPath, base);
    resolveField(cfg.image.imageSavePath,       base);
    resolveField(cfg.display.loadingImagePath,  base);
    resolveField(cfg.display.defaultImagesPath, base);
    // update.backupDirName is a bare name fragment, not a path — not resolved here
}
