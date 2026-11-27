#include "display/PartialRefresh.hpp"
#include "display/DisplayManager.hpp"
#include <algorithm>

namespace partial_refresh
{
std::optional<Window> changedWindow(const std::vector<uint8_t>& shown, const std::vector<uint8_t>& next)
{
    constexpr int kRowBytes = DisplayManager::WIDTH / 8;
    if (shown.size() != next.size() || next.size() != DisplayManager::BUFFER_SIZE)
        return Window{0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT};

    std::optional<Window> window;
    for (size_t i = 0; i < next.size(); ++i)
    {
        if (shown[i] == next[i])
            continue;
        const int x = static_cast<int>(i % kRowBytes) * 8;
        const int y = static_cast<int>(i / kRowBytes);
        if (!window)
            window = Window{x, y, x + 8, y + 1};
        else
            *window = {std::min(window->x0, x), std::min(window->y0, y), std::max(window->x1, x + 8),
                       std::max(window->y1, y + 1)};
    }
    return window;
}

Kind choose(const std::vector<uint8_t>& shown, const std::optional<Window>& changed, const int partialsInARow,
            const Limits& limits)
{
    if (shown.empty())
        return Kind::Full;
    if (!changed)
        return Kind::None;
    constexpr int64_t kPanelArea = int64_t{DisplayManager::WIDTH} * DisplayManager::HEIGHT;
    if (!limits.enabled || partialsInARow >= limits.maxInARow || changed->area() * 100 > kPanelArea * limits.maxAreaPercent)
        return Kind::Full;
    return Kind::Partial;
}
} // namespace partial_refresh
