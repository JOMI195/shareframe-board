#include "display/DisplayManager.hpp"
#include "display/PanelText.hpp"
#include "display/RegistrationScreenRenderer.hpp"
#include <gtest/gtest.h>

namespace
{
int blackIn(const std::vector<uint8_t>& buffer, const panel_text::Rect& r)
{
    int black = 0;
    for (int y = r.y0; y < r.y1; ++y)
        for (int x = r.x0; x < r.x1; ++x)
        {
            const int i = y * DisplayManager::WIDTH + x;
            black += !(buffer[i / 8] & (0x80 >> (i % 8)));
        }
    return black;
}
}

TEST(RegistrationScreenRenderer, QrAndSerialStayInsideTheVisibleArea)
{
    const auto buffer = RegistrationScreenRenderer::render(
        "https://www.shareframe.de/bilderrahmen?register=EOJ8-WXBV-Y86U-3LL3", "www.shareframe.de",
        "EOJ8-WXBV-Y86U-3LL3", std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans.ttf");
    ASSERT_EQ(buffer.size(), DisplayManager::BUFFER_SIZE);

    constexpr auto safe = panel_text::SAFE_AREA;
    EXPECT_EQ(blackIn(buffer, safe), blackIn(buffer, {0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT}))
        << "ink behind the passepartout";
    const int middle = (safe.x0 + safe.x1) / 2;
    EXPECT_GT(blackIn(buffer, {safe.x0, safe.y0, middle - 50, safe.y1}), 0) << "no QR code";
    EXPECT_GT(blackIn(buffer, {middle + 50, safe.y0, safe.x1, safe.y1}), 0) << "no text";
}
