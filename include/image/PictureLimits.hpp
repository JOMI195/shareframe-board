#pragma once
#include <cstdint>

namespace picture
{
/// Largest picture the board decodes: a 16 MP RGB decode stays well inside the Pi Zero's memory.
inline constexpr int64_t kMaxPixels = 16'000'000;
inline constexpr std::uintmax_t kMaxFileBytes = 20 * 1024 * 1024;
}
