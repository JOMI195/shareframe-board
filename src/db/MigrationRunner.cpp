#include "db/MigrationRunner.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

MigrationRunner::MigrationRunner(SQLite::Database& db, std::filesystem::path migrationsDir)
    : _db(db), _migrationsDir(std::move(migrationsDir))
{
}

void MigrationRunner::run() const
{
    ensureMigrationsTable();

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(_migrationsDir))
    {
        if (entry.path().extension() == ".sql")
            files.push_back(entry.path());
    }
    std::ranges::sort(files);

    for (const auto& path : files)
    {
        const std::string id = path.stem().string();
        if (isApplied(id))
        {
            spdlog::debug("Migration already applied, skipping: {}", id);
            continue;
        }

        spdlog::info("Applying migration: {}", id);
        std::ifstream f(path);
        if (!f)
            throw std::runtime_error("Cannot open migration file: " + path.string());
        std::ostringstream ss;
        ss << f.rdbuf();

        apply(id, ss.str());
        spdlog::info("Migration applied: {}", id);
    }
}

void MigrationRunner::ensureMigrationsTable() const
{
    _db.exec(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  id         TEXT    PRIMARY KEY,"
        "  applied_at INTEGER NOT NULL"
        ")"
    );
}

bool MigrationRunner::isApplied(const std::string& id) const
{
    SQLite::Statement q(_db, "SELECT 1 FROM schema_migrations WHERE id = ?");
    q.bind(1, id);
    return q.executeStep();
}

void MigrationRunner::apply(const std::string& id, const std::string& sql) const
{
    SQLite::Transaction tx(_db);
    _db.exec(sql);
    SQLite::Statement ins(_db, "INSERT INTO schema_migrations (id, applied_at) VALUES (?, unixepoch())");
    ins.bind(1, id);
    ins.exec();
    tx.commit();
}
