#pragma once
#include <cstdint>
#include <span>

namespace panel_similarity
{
/// How far apart two rendered panel buffers look, 0 (same) to 1 (inverted). Compared as the share of black
/// in coarse blocks, so two dither patterns of the same photo count as the same.
[[nodiscard]] double difference(std::span<const uint8_t> a, std::span<const uint8_t> b);

/// Below this two pictures put nearly the same charge pattern on the e-ink, wearing it like one held picture.
inline constexpr double kMinDifference = 0.08;
} // namespace panel_similarity
