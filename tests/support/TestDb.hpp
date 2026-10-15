#pragma once
#include "TempDir.hpp"
#include "db/Database.hpp"

class TestDb
{
public:
    TestDb()
    {
        DatabaseConfig cfg;
        cfg.databasePath = dir_.path().string();
        cfg.databaseName = "test.db";
        cfg.migrationsPath = SHAREFRAME_MIGRATIONS_DIR;
        db_.init(cfg, true);
    }

    [[nodiscard]] SQLite::Database& get() const { return db_.get(); }
    [[nodiscard]] const TempDir& dir() const { return dir_; }

private:
    TempDir dir_;
    Database db_;
};
