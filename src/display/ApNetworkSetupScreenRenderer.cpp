#include "display/ApNetworkSetupScreenRenderer.hpp"
#include <fstream>
#include <iterator>
#include <qrcodegen.hpp>
#include <spdlog/spdlog.h>
#include <stb_truetype.h>

namespace
{
using Rect = ApNetworkSetupScreenRenderer::Rect;
constexpr Rect kSafe = ApNetworkSetupScreenRenderer::SAFE_AREA;
constexpr int kQrBox = 300;
constexpr int kQrQuietModules = 2;
constexpr int kColumnGap = 20;
constexpr int kTitlePx = 34;
constexpr int kBodyPx = 24;
constexpr int kMinBodyPx = 16;

const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("ApNetworkSetupScreen");
    return instance;
}

class Canvas
{
public:
    Canvas() : buffer_(DisplayManager::BUFFER_SIZE, 0xFF) {}

    void black(const int x, const int y)
    {
        if (x < kSafe.x0 || x >= kSafe.x1 || y < kSafe.y0 || y >= kSafe.y1)
            return;
        const int i = y * DisplayManager::WIDTH + x;
        buffer_[i / 8] &= static_cast<uint8_t>(~(0x80 >> (i % 8)));
    }

    std::vector<uint8_t> take() { return std::move(buffer_); }

private:
    std::vector<uint8_t> buffer_;
};

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

class Font
{
public:
    bool load(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary);
        data_.assign(std::istreambuf_iterator<char>(f), {});
        return !data_.empty() && stbtt_InitFont(&info_, data_.data(), stbtt_GetFontOffsetForIndex(data_.data(), 0));
    }

    float scale(const int px) const { return stbtt_ScaleForPixelHeight(&info_, static_cast<float>(px)); }

    int lineHeight(const int px) const
    {
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&info_, &ascent, &descent, &gap);
        return static_cast<int>((ascent - descent + gap) * scale(px) + 0.5f);
    }

    int ascent(const int px) const
    {
        int ascent, descent, gap;
        stbtt_GetFontVMetrics(&info_, &ascent, &descent, &gap);
        return static_cast<int>(ascent * scale(px) + 0.5f);
    }

    float width(const std::u32string& text, const int px) const
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

    void draw(Canvas& canvas, const std::u32string& text, const int px, const int left, const int baseline) const
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

private:
    std::vector<unsigned char> data_;
    stbtt_fontinfo info_{};
};

/// Greedy word wrap; a word wider than a line (long SSID/password) is broken anywhere.
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

void drawQr(Canvas& canvas, const std::string& payload)
{
    const auto qr = qrcodegen::QrCode::encodeText(payload.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int size = qr.getSize();
    const int module = kQrBox / (size + 2 * kQrQuietModules);
    const int boxTop = kSafe.y0 + (kSafe.y1 - kSafe.y0 - kQrBox) / 2;
    const int left = kSafe.x0 + (kQrBox - size * module) / 2;
    const int top = boxTop + (kQrBox - size * module) / 2;
    for (int my = 0; my < size; ++my)
        for (int mx = 0; mx < size; ++mx)
            if (qr.getModule(mx, my))
                for (int y = 0; y < module; ++y)
                    for (int x = 0; x < module; ++x)
                        canvas.black(left + mx * module + x, top + my * module + y);
}

void drawText(Canvas& canvas, const Font& font, const std::vector<std::string>& paragraphs)
{
    const int left = kSafe.x0 + kQrBox + kColumnGap;
    const int maxWidth = kSafe.x1 - left;
    const int maxHeight = kSafe.y1 - kSafe.y0;

    for (int body = kBodyPx; body >= kMinBodyPx; body -= 2)
    {
        const int title = kTitlePx * body / kBodyPx;
        const int gap = body / 2;
        std::vector<std::pair<std::vector<std::u32string>, int>> blocks;
        int height = 0;
        for (size_t i = 0; i < paragraphs.size(); ++i)
        {
            const int px = i == 0 ? title : body;
            auto lines = wrap(font, decodeUtf8(paragraphs[i]), px, maxWidth);
            height += static_cast<int>(lines.size()) * font.lineHeight(px) + (i > 0 ? gap : 0);
            blocks.emplace_back(std::move(lines), px);
        }
        if (height > maxHeight && body - 2 >= kMinBodyPx)
            continue;

        int y = kSafe.y0 + std::max(0, (maxHeight - height) / 2);
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            const auto& [lines, px] = blocks[i];
            if (i > 0)
                y += gap;
            for (const auto& line : lines)
            {
                font.draw(canvas, line, px, left, y + font.ascent(px));
                y += font.lineHeight(px);
            }
        }
        return;
    }
}
} // namespace

std::string ApNetworkSetupScreenRenderer::wifiQrPayload(const std::string& ssid, const std::string& password)
{
    auto escape = [](const std::string& s)
    {
        std::string out;
        for (const char c : s)
        {
            if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"')
                out += '\\';
            out += c;
        }
        return out;
    };
    if (password.empty())
        return "WIFI:T:nopass;S:" + escape(ssid) + ";;";
    return "WIFI:T:WPA;S:" + escape(ssid) + ";P:" + escape(password) + ";;";
}

std::vector<uint8_t> ApNetworkSetupScreenRenderer::render(const std::string& ssid, const std::string& password,
                                                          const std::string& url, const std::string& fontPath)
{
    Canvas canvas;
    drawQr(canvas, wifiQrPayload(ssid, password));

    if (Font font; font.load(fontPath))
    {
        const std::string join = password.empty()
                                     ? "mit WLAN " + ssid + " verbinden"
                                     : "mit WLAN " + ssid + " verbinden (Passwort " + password + ")";
        drawText(canvas, font, {
                     "Rahmen einrichten",
                     "1. QR-Code scannen oder " + join,
                     "2. Einrichtung öffnet sich automatisch, sonst " + url
                 });
    }
    else
        logger()->error("Failed to load font {}, setup screen shows the QR code only", fontPath);

    return canvas.take();
}
