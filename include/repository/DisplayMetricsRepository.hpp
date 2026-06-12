#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

/// Persistent key/value counters for e-paper wear/usage. All access is serialized
/// by an internal mutex: the display process shares one SQLite connection across
/// several threads.
class DisplayMetricsRepository
{
public:
    explicit DisplayMetricsRepository(SQLite::Database& db);

    /// Atomically add `by` to a counter (creating it at `by` if absent).
    void increment(const std::string& key, int64_t by = 1);

    /// Overwrite a value (used for last-refresh gauges / timestamps).
    void set(const std::string& key, int64_t value);

    /// Set only if the key does not yet exist (used for first-use timestamp).
    void setIfUnset(const std::string& key, int64_t value);

    /// Read a single counter; 0 when absent.
    [[nodiscard]] int64_t get(const std::string& key) const;

    /// Snapshot of every stored counter, for the stats endpoint.
    [[nodiscard]] std::map<std::string, int64_t> all() const;

private:
    SQLite::Database& db_;
    mutable std::mutex mtx_;
};
