#include "config/ConfigLoader.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

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

    // Secrets
    SecretsConfig secrets;
    auto serr = glz::read_file_toml(secrets, secretsFilePath, std::string{});
    if (serr)
        throw std::runtime_error(
            std::string("Failed to load secrets '") + secretsFilePath
            + "': " + glz::format_error(serr, ""));
    cfg.secrets = secrets;

    // Version
    VersionConfig ver;
    auto verr = readTomlFile(ver, versionFilePath);
    if (verr)
        throw std::runtime_error(
            std::string("Failed to load version '") + versionFilePath
            + "': " + glz::format_error(verr, ""));
    cfg.version = ver.version;

    resolvePaths(cfg);
    return cfg;
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
