#include "display/QrLayout.hpp"
#include <algorithm>
#include <qrcodegen.hpp>

namespace qr_layout
{
namespace
{
using namespace panel_text;
constexpr Rect kSafe = SAFE_AREA;
constexpr int kQrBox = 300;
constexpr int kQrQuietModules = 2;
constexpr int kColumnGap = 20;
constexpr int kTitlePx = 34;
constexpr int kBodyPx = 24;
constexpr int kMinBodyPx = 16;
} // namespace

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
} // namespace qr_layout
