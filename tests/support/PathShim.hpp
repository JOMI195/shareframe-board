#pragma once
#include "TempDir.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

/// Puts fake commands first on PATH for the lifetime of the object, so code that
/// shells out through Subprocess runs a script instead of the board tool.
class PathShim
{
public:
    PathShim() : oldPath_(std::getenv("PATH") ? std::getenv("PATH") : "")
    {
        ::setenv("PATH", (dir_.path().string() + ":" + oldPath_).c_str(), 1);
    }

    ~PathShim() { ::setenv("PATH", oldPath_.c_str(), 1); }

    void add(const std::string& name, const std::string& script)
    {
        const auto path = dir_ / name;
        std::ofstream(path) << "#!/bin/sh\n" << script << "\n";
        std::filesystem::permissions(path, std::filesystem::perms::owner_all);
    }

private:
    TempDir dir_;
    std::string oldPath_;
};
