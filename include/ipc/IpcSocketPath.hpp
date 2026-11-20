#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <unistd.h>

namespace ipc_socket
{
constexpr std::string_view kScheme = "ipc://";

/// nng's ipc:// transport binds a Unix socket file: its folder has to exist, and a file left by a
/// crashed predecessor would block the bind. Other transports are left alone.
inline void prepare(const std::string& url)
{
    if (!url.starts_with(kScheme))
        return;
    const std::filesystem::path path(url.substr(kScheme.size()));
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    ::unlink(path.c_str());
}

inline void remove(const std::string& url)
{
    if (url.starts_with(kScheme))
        ::unlink(url.substr(kScheme.size()).c_str());
}
} // namespace ipc_socket
