#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <filesystem>
#include <string>

class MigrationRunner
{
public:
    MigrationRunner(SQLite::Database& db, std::filesystem::path migrationsDir);
    void run() const;

private:
    SQLite::Database& _db;
    std::filesystem::path _migrationsDir;

    void ensureMigrationsTable() const;
    bool isApplied(const std::string& id) const;
    void apply(const std::string& id, const std::string& sql) const;
};
