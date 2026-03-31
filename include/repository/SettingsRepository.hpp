#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <string>

class SettingsRepository
{
public:
    explicit SettingsRepository(SQLite::Database& db);

    std::optional<std::string> get(const std::string& key) const;
    void set(const std::string& key, const std::string& value) const;

private:
    SQLite::Database& db_;
};
