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
constexpr float STRETCH_LOW_PERCENTILE = 0.01f;
constexpr float STRETCH_HIGH_PERCENTILE = 0.99f;
constexpr int STRETCH_MIN_SPREAD = 32;
constexpr float GAMMA = 0.9f;

const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("PhotoRenderer");
    return instance;
}

// Stretches the 1st..99th luma percentile to full range, skipped for near-flat
// images where it would only amplify noise, then lifts the midtones slightly.
void applyToneCurve(std::vector<float>& gray)
{
    std::array<int, 256> histogram{};
    for (const float v : gray)
        ++histogram[std::clamp(static_cast<int>(v), 0, 255)];

    const auto percentile = [&](const float p)
    {
        const auto target = static_cast<int>(p * static_cast<float>(gray.size()));
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
        const float t = stretch ? (v - low) / static_cast<float>(high - low) : v / 255.0f;
        v = std::pow(std::clamp(t, 0.0f, 1.0f), GAMMA) * 255.0f;
    }
}
} // namespace

std::vector<uint8_t> PhotoRenderer::render(const std::filesystem::path& imagePath)
{
    int w, h, channels;
    auto* data = stbi_load(imagePath.c_str(), &w, &h, &channels, 3);
    if (!data)
    {
        logger()->error("Failed to load image: {} ({})",
                       imagePath.string(), stbi_failure_reason());
        return {};
    }

    // Resize to display dimensions
    std::vector<uint8_t> resized(DisplayManager::WIDTH * DisplayManager::HEIGHT * 3);
    stbir_resize_uint8_linear(
        data, w, h, w * 3,
        resized.data(), DisplayManager::WIDTH, DisplayManager::HEIGHT, DisplayManager::WIDTH * 3,
        STBIR_RGB);
    stbi_image_free(data);

    // Convert to grayscale
    std::vector<float> gray(DisplayManager::WIDTH * DisplayManager::HEIGHT);
    for (int i = 0; i < DisplayManager::WIDTH * DisplayManager::HEIGHT; ++i)
    {
        const float r = resized[i * 3 + 0];
        const float g = resized[i * 3 + 1];
        const float b = resized[i * 3 + 2];
        gray[i] = 0.299f * r + 0.587f * g + 0.114f * b;
    }

    applyToneCurve(gray);

    // Serpentine Floyd-Steinberg: odd rows run right to left with the kernel mirrored
    for (int y = 0; y < DisplayManager::HEIGHT; ++y)
    {
        const bool reverse = y % 2 == 1;
        const int dir = reverse ? -1 : 1;
        for (int i = 0; i < DisplayManager::WIDTH; ++i)
        {
            const int x = reverse ? DisplayManager::WIDTH - 1 - i : i;
            const int idx = y * DisplayManager::WIDTH + x;
            const float oldPixel = gray[idx];
            const float newPixel = (oldPixel > 127.5f) ? 255.0f : 0.0f;
            gray[idx] = newPixel;
            float error = oldPixel - newPixel;

            auto distribute = [&](const int dx, const int dy, const float factor)
            {
                const int ny = y + dy;
                if (const int nx = x + dx * dir; nx >= 0 && nx < DisplayManager::WIDTH && ny >= 0 && ny < DisplayManager::HEIGHT)
                    gray[ny * DisplayManager::WIDTH + nx] += error * factor;
            };
            distribute(1, 0, 7.0f / 16.0f);
            distribute(-1, 1, 3.0f / 16.0f);
            distribute(0, 1, 5.0f / 16.0f);
            distribute(1, 1, 1.0f / 16.0f);
        }
    }

    // Pack into 1-bit buffer (MSB first, white=1 black=0)
    std::vector<uint8_t> buffer(DisplayManager::BUFFER_SIZE, 0x00);
    for (int i = 0; i < DisplayManager::WIDTH * DisplayManager::HEIGHT; ++i)
    {
        if (gray[i] > 127.5f)
            buffer[i / 8] |= (0x80 >> (i % 8));
    }

    logger()->debug("Image buffer prepared: {} bytes", buffer.size());
    return buffer;
}
