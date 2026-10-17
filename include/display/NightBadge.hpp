#pragma once
#include "display/PanelText.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// A small note on the held photo that night mode is on, so nobody wonders
/// why the picture doesn't change.
namespace night_badge
{
/// Where the badge sits: the bottom-right corner of the visible area.
inline constexpr panel_text::Rect BADGE_AREA{panel_text::SAFE_AREA.x1 - 300, panel_text::SAFE_AREA.y1 - 44,
                                             panel_text::SAFE_AREA.x1, panel_text::SAFE_AREA.y1};

std::string text(int endHour);

/// Draws `text` boxed into BADGE_AREA; the buffer is left as is when the font fails to load.
void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text);
} // namespace night_badge
