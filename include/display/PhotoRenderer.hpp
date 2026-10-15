#pragma once
#include "util/NonInstantiable.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

class PhotoRenderer : NonInstantiable
{
public:
    /// Decode, resize to the panel and dither to its 1-bit buffer; empty on failure.
    [[nodiscard]] static std::vector<uint8_t> render(const std::filesystem::path& imagePath);
};
