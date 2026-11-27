#pragma once
#include <cstdint>
#include <optional>
#include <vector>

/// Whether a new panel buffer goes up as a full refresh or only refreshes the window that changed.
namespace partial_refresh
{
/// Panel pixels [x0, x1) × [y0, y1); x0 and x1 are multiples of 8, as the controller wants them.
struct Window
{
    int x0, y0, x1, y1;
    [[nodiscard]] int64_t area() const { return int64_t{x1 - x0} * (y1 - y0); }
    bool operator==(const Window&) const = default;
};

/// The smallest window holding every pixel that differs; nothing when the buffers match.
/// Both are full panel buffers.
[[nodiscard]] std::optional<Window> changedWindow(const std::vector<uint8_t>& shown,
                                                  const std::vector<uint8_t>& next);

enum class Kind
{
    None, ///< the panel already shows it
    Partial,
    Full
};

struct Limits
{
    bool enabled = true;
    /// Partials in a row before a full refresh clears their ghosting.
    int maxInARow = 5;
    /// Larger changes refresh the whole panel.
    int maxAreaPercent = 25;
};

/// `shown` is empty when what the panel holds is unknown.
[[nodiscard]] Kind choose(const std::vector<uint8_t>& shown, const std::optional<Window>& changed,
                          int partialsInARow, const Limits& limits);
} // namespace partial_refresh
