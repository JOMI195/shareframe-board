#include "repository/DisplayMetricsRepository.hpp"

DisplayMetricsRepository::DisplayMetricsRepository(SQLite::Database& db) : db_(db)
{
}

void DisplayMetricsRepository::increment(const std::string& key, int64_t by)
{
    std::lock_guard lk(mtx_);
    SQLite::Statement stmt(db_,
        "INSERT INTO display_metrics (key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value = value + excluded.value");
    stmt.bind(1, key);
    stmt.bind(2, by);
    stmt.exec();
}

void DisplayMetricsRepository::set(const std::string& key, int64_t value)
{
    std::lock_guard lk(mtx_);
    SQLite::Statement stmt(db_, "INSERT OR REPLACE INTO display_metrics (key, value) VALUES (?, ?)");
    stmt.bind(1, key);
    stmt.bind(2, value);
    stmt.exec();
}

void DisplayMetricsRepository::setIfUnset(const std::string& key, int64_t value)
{
    std::lock_guard lk(mtx_);
    SQLite::Statement stmt(db_, "INSERT OR IGNORE INTO display_metrics (key, value) VALUES (?, ?)");
    stmt.bind(1, key);
    stmt.bind(2, value);
    stmt.exec();
}

int64_t DisplayMetricsRepository::get(const std::string& key) const
{
    std::lock_guard lk(mtx_);
    SQLite::Statement query(db_, "SELECT value FROM display_metrics WHERE key = ?");
    query.bind(1, key);
    if (query.executeStep())
        return query.getColumn(0).getInt64();
    return 0;
}

std::map<std::string, int64_t> DisplayMetricsRepository::all() const
{
    std::lock_guard lk(mtx_);
    std::map<std::string, int64_t> result;
    SQLite::Statement query(db_, "SELECT key, value FROM display_metrics");
    while (query.executeStep())
        result.emplace(query.getColumn(0).getString(), query.getColumn(1).getInt64());
    return result;
}
