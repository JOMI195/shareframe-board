#include "display/PhotoRenderer.hpp"
#include "display/DisplayManager.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_resize2.h>

namespace
{
const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("PhotoRenderer");
    return instance;
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

    // Floyd-Steinberg dithering
    for (int y = 0; y < DisplayManager::HEIGHT; ++y)
    {
        for (int x = 0; x < DisplayManager::WIDTH; ++x)
        {
            const int idx = y * DisplayManager::WIDTH + x;
            const float oldPixel = gray[idx];
            const float newPixel = (oldPixel > 127.5f) ? 255.0f : 0.0f;
            gray[idx] = newPixel;
            float error = oldPixel - newPixel;

            auto distribute = [&](const int dx, const int dy, const float factor)
            {
                const int ny = y + dy;
                if (const int nx = x + dx; nx >= 0 && nx < DisplayManager::WIDTH && ny >= 0 && ny < DisplayManager::HEIGHT)
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
