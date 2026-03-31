#include "repository/SettingsRepository.hpp"

SettingsRepository::SettingsRepository(SQLite::Database& db) : db_(db)
{
}

std::optional<std::string> SettingsRepository::get(const std::string& key) const
{
    SQLite::Statement query(db_, "SELECT value FROM settings WHERE key = ?");
    query.bind(1, key);
    if (query.executeStep())
        return query.getColumn(0).getString();
    return std::nullopt;
}

void SettingsRepository::set(const std::string& key, const std::string& value) const
{
    SQLite::Statement stmt(db_, "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
    stmt.bind(1, key);
    stmt.bind(2, value);
    stmt.exec();
}
