#include "db/Database.hpp"
#include "db/MigrationRunner.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <stdexcept>

void Database::init(const DatabaseConfig& config)
{
    std::filesystem::create_directories(config.databasePath);

    const std::string fullPath = config.databasePath + "/" + config.databaseName;
    spdlog::info("Opening database: {}", fullPath);
    _db = std::make_unique<SQLite::Database>(fullPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    _db->exec("PRAGMA journal_mode=WAL");

    spdlog::info("Running migrations from: {}", config.migrationsPath);
    MigrationRunner runner(*_db, config.migrationsPath);
    runner.run();
    spdlog::info("Database ready");
}

SQLite::Database& Database::get() const
{
    if (!_db)
        throw std::runtime_error("Database::get() called before init()");
    return *_db;
}
