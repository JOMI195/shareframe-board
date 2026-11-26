#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace picture
{
enum class Verdict
{
    Ok,
    TooLarge,
    NotAPicture,
    TooManyPixels,
    NoSpace,
};

/// Whether `bytes` may be stored under `storage`, judged from the header alone; nothing is decoded.
[[nodiscard]] Verdict check(std::span<const uint8_t> bytes, const std::filesystem::path& storage, int minFreeMb);

[[nodiscard]] std::string_view describe(Verdict verdict);

/// ".png" or ".jpg", after the bytes themselves.
[[nodiscard]] std::string_view extension(std::span<const uint8_t> bytes);
} // namespace picture
