#pragma once
#include "display/PanelText.hpp"
#include "net/ServerConnection.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// A small note on the photo that the frame has lost the server, so nobody
/// wonders why no new pictures arrive.
namespace offline
{
/// Where the badge sits: the bottom-right corner of the visible area.
inline constexpr panel_text::Rect BADGE_AREA{panel_text::SAFE_AREA.x1 - 260, panel_text::SAFE_AREA.y1 - 44,
                                             panel_text::SAFE_AREA.x1, panel_text::SAFE_AREA.y1};

/// The badge text, nullopt while connected, before the first connection or
/// below `thresholdSecs` offline; a threshold of 0 never shows one.
std::optional<std::string> badgeText(int64_t nowUnix, const ServerConnection& connection, int thresholdSecs);

/// Draws `text` boxed into BADGE_AREA; the buffer is left as is when the font fails to load.
void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text);
} // namespace offline
