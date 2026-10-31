#pragma once
#include "display/PanelText.hpp"
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

/// A small boxed note on the photo, e.g. that night mode holds it, who sent it, or
/// that the frame only shows samples.
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

inline constexpr std::string_view NO_IMAGES_TEXT = "Noch keine Bilder auf dem Rahmen";

/// A `width`×`height` box at a random spot inside the visible area, clear of every rect in
/// `avoid` when such a spot is found. Larger boxes are shrunk to the visible area.
panel_text::Rect randomPlacement(int width, int height, std::mt19937& rng, const std::vector<panel_text::Rect>& avoid);

/// Draws `text` boxed into area(corner), cut short with an ellipsis when it doesn't fit;
/// the buffer is left as is when the font fails to load.
void draw(std::vector<uint8_t>& buffer, const std::string& fontPath, const std::string& text, Corner corner);

/// Draws `text` boxed at a random spot clear of `avoid`; returns where it went, or
/// nothing when the font fails to load.
std::optional<panel_text::Rect> drawAnywhere(std::vector<uint8_t>& buffer, const std::string& fontPath,
                                             const std::string& text, std::mt19937& rng,
                                             const std::vector<panel_text::Rect>& avoid);
} // namespace panel_badge
