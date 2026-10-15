#pragma once
#include <filesystem>
#include <random>
#include <string>

class TempDir
{
public:
    TempDir()
    {
        std::random_device rd;
        path_ = std::filesystem::temp_directory_path() / ("shareframe-test-" + std::to_string(rd()));
        std::filesystem::create_directories(path_);
    }

    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
    [[nodiscard]] std::filesystem::path operator/(const std::string& name) const { return path_ / name; }

private:
    std::filesystem::path path_;
};
