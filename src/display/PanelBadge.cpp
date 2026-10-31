#include "display/PanelBadge.hpp"
#include <spdlog/spdlog.h>

namespace panel_badge
{
namespace
{
constexpr int kPx = 20;
constexpr int kPadding = 8;
constexpr int kBorder = 2;
constexpr int kInset = kPadding + kBorder;
constexpr int kPlacementTries = 64;

bool loadFont(panel_text::Font& font, const std::string& fontPath)
{
    if (font.load(fontPath))
        return true;
    spdlog::error("Failed to load font {}, no badge", fontPath);
    return false;
}

std::u32string fitLine(const panel_text::Font& font, const std::string& text, const int maxBoxWidth)
{
    auto line = panel_text::decodeUtf8(text);
    if (const float maxWidth = static_cast<float>(maxBoxWidth - 2 * kInset); font.width(line, kPx) > maxWidth)
    {
        while (!line.empty() && font.width(line + U"…", kPx) > maxWidth)
            line.pop_back();
        line += U"…";
    }
    return line;
}

int boxWidth(const panel_text::Font& font, const std::u32string& line)
{
    return static_cast<int>(font.width(line, kPx)) + 2 * kInset;
}

int boxHeight(const panel_text::Font& font)
{
    return font.lineHeight(kPx) + 2 * kInset;
}

void drawBox(std::vector<uint8_t>& buffer, const panel_text::Font& font, const std::u32string& line,
             const panel_text::Rect& box)
{
    panel_text::Canvas canvas(std::move(buffer));
    for (int y = box.y0; y < box.y1; ++y)
        for (int x = box.x0; x < box.x1; ++x)
        {
            const bool edge = x < box.x0 + kBorder || x >= box.x1 - kBorder || y < box.y0 + kBorder || y >= box.y1 - kBorder;
            edge ? canvas.black(x, y) : canvas.white(x, y);
        }
    font.draw(canvas, line, kPx, box.x0 + kInset, box.y0 + kInset + font.ascent(kPx));
    buffer = canvas.take();
}

bool overlaps(const panel_text::Rect& a, const panel_text::Rect& b)
{
    return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}
} // namespace

std::string nightText(const int endHour)
{
    return "Nachtmodus bis " + std::to_string(endHour) + " Uhr";
}

panel_text::Rect randomPlacement(int width, int height, std::mt19937& rng, const std::vector<panel_text::Rect>& avoid)
{
    constexpr auto& s = panel_text::SAFE_AREA;
    width = std::min(width, s.x1 - s.x0);
    height = std::min(height, s.y1 - s.y0);
    std::uniform_int_distribution xs(s.x0, s.x1 - width);
    std::uniform_int_distribution ys(s.y0, s.y1 - height);
    auto pick = [&]
    {
        const int x = xs(rng);
        const int y = ys(rng);
        return panel_text::Rect{x, y, x + width, y + height};
    };
    for (int i = 0; i < kPlacementTries; ++i)
        if (const auto r = pick(); std::ranges::none_of(avoid, [&](const auto& a) { return overlaps(r, a); }))
            return r;
    return pick();
}

void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text, const Corner corner)
{
    panel_text::Font font;
    if (!loadFont(font, fontPath))
        return;

    const auto box = area(corner);
    const auto line = fitLine(font, text, box.x1 - box.x0);
    const int x0 = std::max(box.x0, box.x1 - boxWidth(font, line));
    const int height = boxHeight(font);
    const int y0 = corner == Corner::TopRight ? box.y0 : std::max(box.y0, box.y1 - height);
    const int y1 = corner == Corner::TopRight ? std::min(box.y1, box.y0 + height) : box.y1;
    drawBox(buffer, font, line, {x0, y0, box.x1, y1});
}

std::optional<panel_text::Rect> drawAnywhere(std::vector<uint8_t>& buffer, const std::string& fontPath,
                                             const std::string& text, std::mt19937& rng,
                                             const std::vector<panel_text::Rect>& avoid)
{
    panel_text::Font font;
    if (!loadFont(font, fontPath))
        return std::nullopt;

    constexpr auto& s = panel_text::SAFE_AREA;
    const auto line = fitLine(font, text, s.x1 - s.x0);
    const auto box = randomPlacement(boxWidth(font, line), boxHeight(font), rng, avoid);
    drawBox(buffer, font, line, box);
    return box;
}
} // namespace panel_badge
