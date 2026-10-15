#include "display/ApNetworkSetupScreenRenderer.hpp"
#include <gtest/gtest.h>

namespace
{
using Renderer = ApNetworkSetupScreenRenderer;

bool isBlack(const std::vector<uint8_t>& buffer, const int x, const int y)
{
    const int i = y * DisplayManager::WIDTH + x;
    return !(buffer[i / 8] & (0x80 >> (i % 8)));
}

int blackIn(const std::vector<uint8_t>& buffer, const int x0, const int y0, const int x1, const int y1)
{
    int black = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            black += isBlack(buffer, x, y);
    return black;
}
}

TEST(ApNetworkSetupScreenRenderer, LongCredentialsStayInsideTheVisibleArea)
{
    const std::string ssid(32, 'W');
    const std::string password(63, 'M');
    const auto buffer = Renderer::render(ssid, password, "http://192.168.4.1",
                                         std::string(SHAREFRAME_ASSETS_DIR) + "/font/DejaVuSans.ttf");
    ASSERT_EQ(buffer.size(), DisplayManager::BUFFER_SIZE);

    constexpr auto safe = Renderer::SAFE_AREA;
    const int total = blackIn(buffer, 0, 0, DisplayManager::WIDTH, DisplayManager::HEIGHT);
    EXPECT_EQ(blackIn(buffer, safe.x0, safe.y0, safe.x1, safe.y1), total) << "ink behind the passepartout";

    const int middle = (safe.x0 + safe.x1) / 2;
    EXPECT_GT(blackIn(buffer, safe.x0, safe.y0, middle - 50, safe.y1), 0) << "no QR code";
    EXPECT_GT(blackIn(buffer, middle + 50, safe.y0, safe.x1, safe.y1), 0) << "no text";
    EXPECT_EQ(blackIn(buffer, middle + 50, safe.y1 - 10, safe.x1, safe.y1), 0) << "text runs to the bottom edge, likely cut off";
}
