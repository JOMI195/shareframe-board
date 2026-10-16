#include "display/OfflineBadge.hpp"
#include <spdlog/spdlog.h>

namespace offline
{
namespace
{
constexpr int kPx = 20;
constexpr int kPadding = 8;
constexpr int kBorder = 2;
} // namespace

std::optional<std::string> badgeText(const int64_t nowUnix, const ServerConnection& connection,
                                     const int thresholdSecs)
{
    if (thresholdSecs <= 0 || connection.connected || connection.since <= 0)
        return std::nullopt;
    const int64_t secs = nowUnix - connection.since;
    if (secs < thresholdSecs)
        return std::nullopt;

    const int64_t hours = secs / 3600;
    if (hours < 1)
        return "Offline seit " + std::to_string(secs / 60) + " Min.";
    if (hours < 48)
        return "Offline seit " + std::to_string(hours) + " Std.";
    return "Offline seit " + std::to_string(hours / 24) + " Tagen";
}

void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text)
{
    panel_text::Font font;
    if (!font.load(fontPath))
    {
        spdlog::error("Failed to load font {}, no offline badge", fontPath);
        return;
    }

    const auto line = panel_text::decodeUtf8(text);
    const int width = static_cast<int>(font.width(line, kPx)) + 2 * (kPadding + kBorder);
    const int height = font.lineHeight(kPx) + 2 * (kPadding + kBorder);
    const int x1 = BADGE_AREA.x1;
    const int y1 = BADGE_AREA.y1;
    const int x0 = std::max(BADGE_AREA.x0, x1 - width);
    const int y0 = std::max(BADGE_AREA.y0, y1 - height);

    panel_text::Canvas canvas(std::move(buffer));
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
            const bool edge = x < x0 + kBorder || x >= x1 - kBorder || y < y0 + kBorder || y >= y1 - kBorder;
            edge ? canvas.black(x, y) : canvas.white(x, y);
        }
    font.draw(canvas, line, kPx, x0 + kBorder + kPadding, y0 + kBorder + kPadding + font.ascent(kPx));
    buffer = canvas.take();
}
} // namespace offline
