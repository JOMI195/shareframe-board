#include "db/Database.hpp"
#include "db/MigrationRunner.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <stdexcept>

void Database::init(const DatabaseConfig& config, bool shouldRunMigrations)
{
    open(config);

    if (shouldRunMigrations)
        runMigrations(std::filesystem::path(config.migrationsPath));

    spdlog::info("Database ready");
}

void Database::open(const DatabaseConfig& config)
{
    std::filesystem::create_directories(config.databasePath);

    const std::filesystem::path fullPath = std::filesystem::path(config.databasePath) / config.databaseName;
    spdlog::info("Opening database: {}", fullPath.string());
    _db = std::make_unique<SQLite::Database>(fullPath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    _db->setBusyTimeout(5000); // wait up to 5s on a locked DB instead of throwing immediately
    _db->exec("PRAGMA journal_mode=WAL");
}

void Database::runMigrations(const std::filesystem::path& migrationsPath) const
{
    spdlog::info("Running migrations from: {}", migrationsPath.string());
    MigrationRunner runner(*_db, migrationsPath);
    runner.run();
}

SQLite::Database& Database::get() const
{
    if (!_db)
        throw std::runtime_error("Database::get() called before init()");
    return *_db;
}
