#include "util/SysInfo.hpp"
#include <array>
#include <sstream>
#include <string_view>

namespace {
bool isMetric(std::string_view key)
{
    static constexpr std::array suffixes = {"_bytes", "_kb", "_seconds", "_celsius", "_percent", "_mhz", "_dbm", "_fds"};
    static constexpr std::array counters = {"boot_count", "crash_count", "unclean_resets", "oom_kills", "uv_events"};
    for (std::string_view s : suffixes)
        if (key.ends_with(s)) return true;
    for (std::string_view c : counters)
        if (key == c) return true;
    return key.starts_with("load_") || key.starts_with("proc_");
}

nlohmann::json number(const std::string& v)
{
    try { std::size_t p; long long i = std::stoll(v, &p); if (p == v.size()) return i; } catch (...) {}
    try { std::size_t p; double d = std::stod(v, &p);     if (p == v.size()) return d; } catch (...) {}
    return v;
}
} // namespace

nlohmann::json SysInfo::parse(const std::string& output)
{
    nlohmann::json result = nlohmann::json::object();
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line))
    {
        const auto eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        const auto key = line.substr(0, eq);
        const auto val = line.substr(eq + 1);
        if (val == "true" || val == "false")
            result[key] = val == "true";
        else
            result[key] = isMetric(key) ? number(val) : nlohmann::json(val);
    }
    return result;
}
