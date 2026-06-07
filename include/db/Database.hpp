#pragma once
#include "config/AppConfig.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <memory>

class Database
{
public:
    void init(const DatabaseConfig& config, bool runMigrations = true);
    [[nodiscard]] SQLite::Database& get() const;

private:
    void open(const DatabaseConfig& config);
    void runMigrations(const std::filesystem::path& migrationsPath) const;

    std::unique_ptr<SQLite::Database> _db;
};
