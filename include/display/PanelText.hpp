#pragma once
#include "display/DisplayManager.hpp"
#include <cstdint>
#include <stb_truetype.h>
#include <string>
#include <vector>

/// Text and pixel drawing on the 1-bit panel buffer, kept to the area the
/// passepartout leaves visible.
namespace panel_text
{
struct Rect
{
    int x0, y0, x1, y1;
};

/// About 140×90 mm of the 163.2×97.92 mm panel show through the passepartout;
/// 5 mm more per side absorb a frame that isn't mounted dead centre.
inline constexpr Rect SAFE_AREA = []
{
    constexpr double pxPerMm = DisplayManager::WIDTH / 163.2;
    constexpr double visibleW = 140.0 * pxPerMm;
    constexpr double visibleH = 90.0 * pxPerMm;
    constexpr double margin = 5.0 * pxPerMm;
    const int x0 = static_cast<int>((DisplayManager::WIDTH - visibleW) / 2 + margin) + 1;
    const int y0 = static_cast<int>((DisplayManager::HEIGHT - visibleH) / 2 + margin) + 1;
    return Rect{x0, y0, DisplayManager::WIDTH - x0, DisplayManager::HEIGHT - y0};
}();

/// Draws onto a panel buffer (1 = white); pixels outside SAFE_AREA are dropped.
class Canvas
{
public:
    explicit Canvas(std::vector<uint8_t> buffer = std::vector<uint8_t>(DisplayManager::BUFFER_SIZE, 0xFF));

    void black(int x, int y);
    void white(int x, int y);
    std::vector<uint8_t> take() { return std::move(buffer_); }

private:
    std::vector<uint8_t> buffer_;
};

std::u32string decodeUtf8(const std::string& s);

class Font
{
public:
    bool load(const std::string& path);

    [[nodiscard]] float scale(int px) const;
    [[nodiscard]] int lineHeight(int px) const;
    [[nodiscard]] int ascent(int px) const;
    [[nodiscard]] float width(const std::u32string& text, int px) const;
    void draw(Canvas& canvas, const std::u32string& text, int px, int left, int baseline) const;

private:
    std::vector<unsigned char> data_;
    stbtt_fontinfo info_{};
};

/// Greedy word wrap; a word wider than a line is broken anywhere.
std::vector<std::u32string> wrap(const Font& font, const std::u32string& text, int px, int maxWidth);
} // namespace panel_text
