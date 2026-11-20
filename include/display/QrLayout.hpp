#pragma once
#include <optional>
#include "display/PanelText.hpp"
#include <string>
#include <vector>

/// A QR code on the left of the visible area with a text column beside it, shared by
/// the setup screens.
namespace qr_layout
{
void drawQr(panel_text::Canvas& canvas, const std::string& payload);

/// The first paragraph is the title; the text shrinks until it fits next to the QR.
/// The first paragraph is the title; `emphasized` (a code to read off the frame) is set as large as the title.
void drawText(panel_text::Canvas& canvas, const panel_text::Font& font, const std::vector<std::string>& paragraphs,
              std::optional<size_t> emphasized = {});
} // namespace qr_layout
