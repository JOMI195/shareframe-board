#include "display/DisplayManager.hpp"
#include "display/PhotoRenderer.hpp"
#include "support/TempDir.hpp"
#include <array>
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
    std::filesystem::path writeImage(const std::string& name, const std::function<uint8_t(int x)>& shade) const
    {
        const auto path = dir / (name + ".pgm");
        std::ofstream f(path, std::ios::binary);
        f << "P5\n" << W << " " << H << "\n255\n";
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                f.put(static_cast<char>(shade(x)));
        return path;
    }

    TempDir dir;
};

int level(const std::vector<uint8_t>& buffer, const int x, const int y)
{
    const int i = y * W + x;
    return (buffer[i / 4] >> (6 - 2 * (i % 4))) & 0b11;
}

double meanLevel(const std::vector<uint8_t>& buffer, const int x0, const int x1)
{
    double sum = 0;
    for (int y = 0; y < H; ++y)
        for (int x = x0; x < x1; ++x)
            sum += level(buffer, x, y);
    return sum / ((x1 - x0) * H);
}
}

TEST_F(PhotoRendererTest, MonoIsTheDefaultOneBitBuffer)
{
    const auto image = writeImage("gray", [](int) { return 128; });
    EXPECT_EQ(PhotoRenderer::render(image).size(), DisplayManager::BUFFER_SIZE);
    EXPECT_EQ(PhotoRenderer::render(image, ImageQuality::Mono), PhotoRenderer::render(image));
}

TEST_F(PhotoRendererTest, Gray4BufferHoldsTwoBitsPerPixel)
{
    const auto image = writeImage("gray", [](int) { return 128; });
    EXPECT_EQ(PhotoRenderer::render(image, ImageQuality::Gray4).size(), DisplayManager::GRAY4_BUFFER_SIZE);
}

TEST_F(PhotoRendererTest, Gray4MapsBlackAndWhiteToTheOuterCodes)
{
    const auto black = PhotoRenderer::render(writeImage("black", [](int) { return 0; }), ImageQuality::Gray4);
    const auto white = PhotoRenderer::render(writeImage("white", [](int) { return 255; }), ImageQuality::Gray4);
    EXPECT_EQ(black, std::vector<uint8_t>(DisplayManager::GRAY4_BUFFER_SIZE, 0x00));
    EXPECT_EQ(white, std::vector<uint8_t>(DisplayManager::GRAY4_BUFFER_SIZE, 0xFF));
}

TEST_F(PhotoRendererTest, Gray4GradientUsesAllLevelsInOrder)
{
    const auto image = writeImage("gradient", [](const int x) { return static_cast<uint8_t>(x * 255 / (W - 1)); });
    const auto buffer = PhotoRenderer::render(image, ImageQuality::Gray4);

    std::array<int, 4> counts{};
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            ++counts[level(buffer, x, y)];
    for (int l = 0; l < 4; ++l)
        EXPECT_GT(counts[l], W * H / 10) << "level " << l << " barely used";

    constexpr int stripes = 8;
    for (int s = 1; s < stripes; ++s)
        EXPECT_GT(meanLevel(buffer, s * W / stripes, (s + 1) * W / stripes),
                  meanLevel(buffer, (s - 1) * W / stripes, s * W / stripes)) << "stripe " << s;
}

TEST_F(PhotoRendererTest, UnreadableImageRendersNothing)
{
    EXPECT_TRUE(PhotoRenderer::render(dir / "missing.jpg", ImageQuality::Gray4).empty());
}
