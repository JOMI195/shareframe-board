#pragma once
#include "util/NonInstantiable.hpp"
#include <string>

class FileHash : NonInstantiable
{
public:
    // SHA-256 of a file, streamed in chunks; lowercase hex, "" on error.
    static std::string sha256File(const std::string& path);
};
