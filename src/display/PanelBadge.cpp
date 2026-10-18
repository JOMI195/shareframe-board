#include "display/PanelBadge.hpp"
#include <spdlog/spdlog.h>

namespace panel_badge
{
namespace
{
constexpr int kPx = 20;
constexpr int kPadding = 8;
constexpr int kBorder = 2;
} // namespace

std::string nightText(const int endHour)
{
    return "Nachtmodus bis " + std::to_string(endHour) + " Uhr";
}

void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text, const Corner corner)
{
    panel_text::Font font;
    if (!font.load(fontPath))
    {
        spdlog::error("Failed to load font {}, no badge", fontPath);
        return;
    }

    const auto box = area(corner);
    const int inset = kPadding + kBorder;
    auto line = panel_text::decodeUtf8(text);
    if (const float maxWidth = static_cast<float>(box.x1 - box.x0 - 2 * inset); font.width(line, kPx) > maxWidth)
    {
        while (!line.empty() && font.width(line + U"…", kPx) > maxWidth)
            line.pop_back();
        line += U"…";
    }

    const int width = static_cast<int>(font.width(line, kPx)) + 2 * inset;
    const int height = font.lineHeight(kPx) + 2 * inset;
    const int x1 = box.x1;
    const int x0 = std::max(box.x0, x1 - width);
    const int y0 = corner == Corner::TopRight ? box.y0 : std::max(box.y0, box.y1 - height);
    const int y1 = corner == Corner::TopRight ? std::min(box.y1, box.y0 + height) : box.y1;

    panel_text::Canvas canvas(std::move(buffer));
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
            const bool edge = x < x0 + kBorder || x >= x1 - kBorder || y < y0 + kBorder || y >= y1 - kBorder;
            edge ? canvas.black(x, y) : canvas.white(x, y);
        }
    font.draw(canvas, line, kPx, x0 + inset, y0 + inset + font.ascent(kPx));
    buffer = canvas.take();
}
} // namespace panel_badge
