#include "display/PhotoRenderer.hpp"
#include "display/DisplayManager.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr int W = DisplayManager::WIDTH;
constexpr int H = DisplayManager::HEIGHT;
constexpr int PIXELS = W * H;

constexpr float STRETCH_LOW_PERCENTILE = 0.01f;
constexpr float STRETCH_HIGH_PERCENTILE = 0.99f;
constexpr int STRETCH_MIN_SPREAD = 32;
constexpr float GRAY4_GAMMA = 0.9f;

// Panel codes for black, dark gray, light gray, white.
constexpr std::array<uint8_t, 4> GRAY4_CODES = {0b00, 0b01, 0b10, 0b11};

const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("PhotoRenderer");
    return instance;
}

std::vector<float> loadGray(const std::filesystem::path& imagePath)
{
    int w, h, channels;
    auto* data = stbi_load(imagePath.c_str(), &w, &h, &channels, 3);
    if (!data)
    {
        logger()->error("Failed to load image: {} ({})",
                       imagePath.string(), stbi_failure_reason());
        return {};
    }

    std::vector<uint8_t> resized(PIXELS * 3);
    stbir_resize_uint8_linear(
        data, w, h, w * 3,
        resized.data(), W, H, W * 3,
        STBIR_RGB);
    stbi_image_free(data);

    std::vector<float> gray(PIXELS);
    for (int i = 0; i < PIXELS; ++i)
    {
        const float r = resized[i * 3 + 0];
        const float g = resized[i * 3 + 1];
        const float b = resized[i * 3 + 2];
        gray[i] = 0.299f * r + 0.587f * g + 0.114f * b;
    }
    return gray;
}

std::vector<uint8_t> renderMono(std::vector<float> gray)
{
    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            const int idx = y * W + x;
            const float oldPixel = gray[idx];
            const float newPixel = (oldPixel > 127.5f) ? 255.0f : 0.0f;
            gray[idx] = newPixel;
            float error = oldPixel - newPixel;

            auto distribute = [&](const int dx, const int dy, const float factor)
            {
                const int ny = y + dy;
                if (const int nx = x + dx; nx >= 0 && nx < W && ny >= 0 && ny < H)
                    gray[ny * W + nx] += error * factor;
            };
            distribute(1, 0, 7.0f / 16.0f);
            distribute(-1, 1, 3.0f / 16.0f);
            distribute(0, 1, 5.0f / 16.0f);
            distribute(1, 1, 1.0f / 16.0f);
        }
    }

    // MSB first, white=1 black=0
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0x00);
    for (int i = 0; i < PIXELS; ++i)
    {
        if (gray[i] > 127.5f)
            buffer[i / 8] |= (0x80 >> (i % 8));
    }
    return buffer;
}

void applyToneCurve(std::vector<float>& gray)
{
    std::array<int, 256> histogram{};
    for (const float v : gray)
        ++histogram[std::clamp(static_cast<int>(v), 0, 255)];

    const auto percentile = [&](const float p)
    {
        const int target = static_cast<int>(p * PIXELS);
        int count = 0;
        for (int v = 0; v < 256; ++v)
        {
            count += histogram[v];
            if (count > target)
                return v;
        }
        return 255;
    };
    const int low = percentile(STRETCH_LOW_PERCENTILE);
    const int high = percentile(STRETCH_HIGH_PERCENTILE);
    const bool stretch = high - low >= STRETCH_MIN_SPREAD;

    for (float& v : gray)
    {
        float t = stretch ? (v - low) / static_cast<float>(high - low) : v / 255.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        v = std::pow(t, GRAY4_GAMMA) * 255.0f;
    }
}

std::vector<uint8_t> renderGray4(std::vector<float> gray)
{
    applyToneCurve(gray);

    constexpr float STEP = 255.0f / 3.0f;
    std::vector<uint8_t> levels(PIXELS);
    for (int y = 0; y < H; ++y)
    {
        const bool reverse = y % 2 == 1;
        const int dir = reverse ? -1 : 1;
        for (int i = 0; i < W; ++i)
        {
            const int x = reverse ? W - 1 - i : i;
            const int idx = y * W + x;
            const float oldPixel = gray[idx];
            const int level = std::clamp(static_cast<int>(std::lround(oldPixel / STEP)), 0, 3);
            levels[idx] = static_cast<uint8_t>(level);
            const float error = oldPixel - level * STEP;

            auto distribute = [&](const int dx, const int dy, const float factor)
            {
                const int ny = y + dy;
                if (const int nx = x + dx * dir; nx >= 0 && nx < W && ny >= 0 && ny < H)
                    gray[ny * W + nx] += error * factor;
            };
            distribute(1, 0, 7.0f / 16.0f);
            distribute(-1, 1, 3.0f / 16.0f);
            distribute(0, 1, 5.0f / 16.0f);
            distribute(1, 1, 1.0f / 16.0f);
        }
    }

    // 4 pixels per byte, first pixel in the top bits
    std::vector<uint8_t> buffer(DisplayManager::GRAY4_BUFFER_SIZE, 0x00);
    for (int i = 0; i < PIXELS; ++i)
        buffer[i / 4] |= GRAY4_CODES[levels[i]] << (6 - 2 * (i % 4));
    return buffer;
}
} // namespace

std::vector<uint8_t> PhotoRenderer::render(const std::filesystem::path& imagePath, const ImageQuality quality)
{
    auto gray = loadGray(imagePath);
    if (gray.empty())
        return {};

    auto buffer = quality == ImageQuality::Gray4 ? renderGray4(std::move(gray)) : renderMono(std::move(gray));
    logger()->debug("Image buffer prepared ({}): {} bytes", toString(quality), buffer.size());
    return buffer;
}
