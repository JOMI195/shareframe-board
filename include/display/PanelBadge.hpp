#pragma once
#include "display/PanelText.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// A small boxed note in a corner of the photo, e.g. that night mode holds it
/// or who sent it.
namespace panel_badge
{
enum class Corner
{
    TopRight,
    BottomRight
};

/// Where a badge in `corner` may sit inside the visible area.
constexpr panel_text::Rect area(const Corner corner)
{
    constexpr auto& s = panel_text::SAFE_AREA;
    return corner == Corner::TopRight ? panel_text::Rect{s.x1 - 300, s.y0, s.x1, s.y0 + 44}
                                      : panel_text::Rect{s.x1 - 300, s.y1 - 44, s.x1, s.y1};
}

std::string nightText(int endHour);

/// Draws `text` boxed into area(corner), cut short with an ellipsis when it doesn't fit;
/// the buffer is left as is when the font fails to load.
void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text, Corner corner);
} // namespace panel_badge
