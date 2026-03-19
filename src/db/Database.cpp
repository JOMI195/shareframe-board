#include "db/Database.hpp"
#include <filesystem>
#include <stdexcept>

void Database::init(const DatabaseConfig& config)
{
    std::filesystem::create_directories(config.databasePath);

    const std::string fullPath = config.databasePath + "/" + config.databaseName;
    _db = std::make_unique<SQLite::Database>(fullPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
}

SQLite::Database& Database::get() const
{
    if (!_db)
        throw std::runtime_error("Database::get() called before init()");
    return *_db;
}
