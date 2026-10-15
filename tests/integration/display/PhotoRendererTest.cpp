#include "display/DisplayManager.hpp"
#include "display/PhotoRenderer.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>

namespace
{
constexpr int W = DisplayManager::WIDTH;
constexpr int H = DisplayManager::HEIGHT;

class PhotoRendererTest : public ::testing::Test
{
protected:
    std::filesystem::path writeImage(const std::function<uint8_t(int x)>& shade) const
    {
        const auto path = dir / "image.pgm";
        std::ofstream f(path, std::ios::binary);
        f << "P5\n" << W << " " << H << "\n255\n";
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                f.put(static_cast<char>(shade(x)));
        return path;
    }

    TempDir dir;
};

double blackRatio(const std::vector<uint8_t>& buffer, const int x0, const int x1)
{
    int black = 0;
    for (int y = 0; y < H; ++y)
        for (int x = x0; x < x1; ++x)
        {
            const int i = y * W + x;
            black += !(buffer[i / 8] & (0x80 >> (i % 8)));
        }
    return static_cast<double>(black) / ((x1 - x0) * H);
}
}

TEST_F(PhotoRendererTest, LowContrastPhotoIsStretchedToFullRange)
{
    const auto buffer = PhotoRenderer::render(writeImage([](const int x) { return 96 + x * 64 / (W - 1); }));
    ASSERT_EQ(buffer.size(), DisplayManager::BUFFER_SIZE);

    EXPECT_GT(blackRatio(buffer, 0, 50), 0.9);
    EXPECT_LT(blackRatio(buffer, W - 50, W), 0.1);
}

TEST_F(PhotoRendererTest, FlatImageStaysMidGray)
{
    const auto buffer = PhotoRenderer::render(writeImage([](int) { return 128; }));
    ASSERT_EQ(buffer.size(), DisplayManager::BUFFER_SIZE);

    const double black = blackRatio(buffer, 0, W);
    EXPECT_GT(black, 0.35);
    EXPECT_LT(black, 0.6);
}
