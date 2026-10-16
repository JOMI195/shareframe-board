#include "display/PanelText.hpp"
#include <fstream>
#include <iterator>

namespace panel_text
{
Canvas::Canvas(std::vector<uint8_t> buffer) : buffer_(std::move(buffer))
{
}

void Canvas::black(const int x, const int y)
{
    if (x < SAFE_AREA.x0 || x >= SAFE_AREA.x1 || y < SAFE_AREA.y0 || y >= SAFE_AREA.y1)
        return;
    const int i = y * DisplayManager::WIDTH + x;
    buffer_[i / 8] &= static_cast<uint8_t>(~(0x80 >> (i % 8)));
}

void Canvas::white(const int x, const int y)
{
    if (x < SAFE_AREA.x0 || x >= SAFE_AREA.x1 || y < SAFE_AREA.y0 || y >= SAFE_AREA.y1)
        return;
    const int i = y * DisplayManager::WIDTH + x;
    buffer_[i / 8] |= static_cast<uint8_t>(0x80 >> (i % 8));
}

std::u32string decodeUtf8(const std::string& s)
{
    std::u32string out;
    for (size_t i = 0; i < s.size();)
    {
        const auto c = static_cast<unsigned char>(s[i]);
        const int len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        char32_t cp = len == 1 ? c : c & (0x7F >> len);
        for (int k = 1; k < len && i + k < s.size(); ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        out.push_back(cp);
        i += len;
    }
    return out;
}

bool Font::load(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    data_.assign(std::istreambuf_iterator<char>(f), {});
    return !data_.empty() && stbtt_InitFont(&info_, data_.data(), stbtt_GetFontOffsetForIndex(data_.data(), 0));
}

float Font::scale(const int px) const
{
    return stbtt_ScaleForPixelHeight(&info_, static_cast<float>(px));
}

int Font::lineHeight(const int px) const
{
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&info_, &ascent, &descent, &gap);
    return static_cast<int>((ascent - descent + gap) * scale(px) + 0.5f);
}

int Font::ascent(const int px) const
{
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&info_, &ascent, &descent, &gap);
    return static_cast<int>(ascent * scale(px) + 0.5f);
}

float Font::width(const std::u32string& text, const int px) const
{
    const float s = scale(px);
    float w = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        int advance, bearing;
        stbtt_GetCodepointHMetrics(&info_, static_cast<int>(text[i]), &advance, &bearing);
        w += advance * s;
        if (i + 1 < text.size())
            w += stbtt_GetCodepointKernAdvance(&info_, static_cast<int>(text[i]), static_cast<int>(text[i + 1])) * s;
    }
    return w;
}

void Font::draw(Canvas& canvas, const std::u32string& text, const int px, const int left, const int baseline) const
{
    const float s = scale(px);
    float x = static_cast<float>(left);
    for (size_t i = 0; i < text.size(); ++i)
    {
        const int cp = static_cast<int>(text[i]);
        int w, h, xoff, yoff;
        if (unsigned char* bitmap = stbtt_GetCodepointBitmap(&info_, s, s, cp, &w, &h, &xoff, &yoff))
        {
            for (int gy = 0; gy < h; ++gy)
                for (int gx = 0; gx < w; ++gx)
                    if (bitmap[gy * w + gx] >= 128)
                        canvas.black(static_cast<int>(x + 0.5f) + xoff + gx, baseline + yoff + gy);
            stbtt_FreeBitmap(bitmap, nullptr);
        }
        int advance, bearing;
        stbtt_GetCodepointHMetrics(&info_, cp, &advance, &bearing);
        x += advance * s;
        if (i + 1 < text.size())
            x += stbtt_GetCodepointKernAdvance(&info_, cp, static_cast<int>(text[i + 1])) * s;
    }
}

std::vector<std::u32string> wrap(const Font& font, const std::u32string& text, const int px, const int maxWidth)
{
    std::vector<std::u32string> lines;
    std::u32string line;
    auto fits = [&](const std::u32string& candidate) { return font.width(candidate, px) <= maxWidth; };

    size_t pos = 0;
    while (pos < text.size())
    {
        const size_t space = text.find(U' ', pos);
        std::u32string word = text.substr(pos, space == std::u32string::npos ? std::u32string::npos : space - pos);
        pos = space == std::u32string::npos ? text.size() : space + 1;

        if (const auto joined = line.empty() ? word : line + U' ' + word; fits(joined))
        {
            line = joined;
            continue;
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        line.clear();
        for (const char32_t c : word)
        {
            if (!line.empty() && !fits(line + c))
                lines.push_back(std::exchange(line, {}));
            line += c;
        }
    }
    if (!line.empty())
        lines.push_back(std::move(line));
    return lines;
}
} // namespace panel_text
