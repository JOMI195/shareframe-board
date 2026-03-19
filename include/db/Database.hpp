#pragma once
#include "config/AppConfig.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>

class Database
{
public:
    void              init(const DatabaseConfig& config);
    SQLite::Database& get() const;

private:
    std::unique_ptr<SQLite::Database> _db;
};
