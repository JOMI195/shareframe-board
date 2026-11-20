#include "util/PanelCode.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <openssl/rand.h>
#include <sys/stat.h>

namespace panel_code
{
std::string generate()
{
    // Rejection sampling: values above the last whole million would favour the low codes.
    constexpr uint32_t kLimit = 4'294'000'000u;
    uint32_t value = 0;
    do
    {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof value) != 1)
            return {};
    } while (value >= kLimit);
    char code[7];
    std::snprintf(code, sizeof code, "%06u", value % 1'000'000u);
    return code;
}

std::string read(const std::string& path)
{
    std::ifstream in(path);
    std::string code;
    in >> code;
    return code.size() == 6 ? code : std::string{};
}

std::string readOrCreate(const std::string& path)
{
    if (auto existing = read(path); !existing.empty())
        return existing;
    auto code = generate();
    if (code.empty())
        return {};
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    const auto tmp = path + ".tmp";
    const auto old = umask(077);
    {
        std::ofstream out(tmp);
        out << code << "\n";
        if (!out)
            code.clear();
    }
    umask(old);
    if (code.empty() || (std::filesystem::rename(tmp, path, ec), ec))
    {
        std::filesystem::remove(tmp, ec);
        return {};
    }
    return code;
}

std::string grouped(const std::string& code)
{
    return code.size() == 6 ? code.substr(0, 3) + " " + code.substr(3) : code;
}
}
