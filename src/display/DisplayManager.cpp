#include "display/DisplayManager.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <thread>

#ifdef EPD_HARDWARE_ENABLED
extern "C" {
#include "DEV_Config.h"
#include "EPD_7in5_V2.h"
}
#endif

DisplayManager::DisplayManager(const AppConfig& cfg)
    : _cfg(cfg), _logger(spdlog::default_logger()->clone("Display"))
{
    _lastDisplayTime = std::chrono::steady_clock::now()
        - std::chrono::seconds(_cfg.display.minRefreshSecs);
    _logger->info("DisplayManager created (mock={})", _cfg.display.mockDisplay);
}

DisplayManager::~DisplayManager()
{
#ifdef EPD_HARDWARE_ENABLED
    if (_initialized && !_cfg.display.mockDisplay)
    {
        EPD_7IN5_V2_Sleep();
        DEV_Module_Exit();
        _logger->info("Display hardware cleaned up");
    }
#endif
}

void DisplayManager::init()
{
    if (!_cfg.display.mockDisplay)
    {
#ifdef EPD_HARDWARE_ENABLED
        if (!_hwInit())
        {
            _logger->error("Display init failed");
            return;
        }
#else
        _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
        return;
#endif
    }
    else
    {
        _logger->info("[mock] Display init");
    }

    if (!displayImage(std::filesystem::path(_cfg.display.loadingImagePath) /
        "logo-frame-loading-shareframe.jpg"))
        _logger->error("Failed to display loading image");
}

void DisplayManager::clear()
{
    _waitMinRefresh();

    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display clear");
        _lastDisplayTime = std::chrono::steady_clock::now();
        return;
    }

#ifdef EPD_HARDWARE_ENABLED
    if (!_hwInit())
        return;

    const auto t0 = std::chrono::steady_clock::now();
    const UBYTE rc = EPD_7IN5_V2_Clear();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _lastDisplayTime = std::chrono::steady_clock::now();
    _hwSleep();
    if (rc != 0)
        _logger->error("Display clear FAILED (BUSY timeout, {} ms)", ms);
    else
        _logger->info("Display cleared (panel refresh {} ms)", ms);
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
#endif
}

bool DisplayManager::displayImage(const std::filesystem::path& imagePath)
{
    _waitMinRefresh();

    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display image: {}", imagePath.string());
        _lastDisplayTime = std::chrono::steady_clock::now();
        return true;
    }

#ifdef EPD_HARDWARE_ENABLED
    auto buffer = _prepareImageBuffer(imagePath);
    if (buffer.empty())
        return false;

    if (!_hwInit())
        return false;

    _logger->debug("Sending image to panel ({} bytes) and refreshing", buffer.size());
    const auto t0 = std::chrono::steady_clock::now();
    const UBYTE rc = EPD_7IN5_V2_Display(buffer.data());
    const auto refreshMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _lastDisplayTime = std::chrono::steady_clock::now();
    _hwSleep();
    if (rc != 0)
    {
        _logger->error("Panel refresh FAILED (BUSY timeout, {} ms): {}", refreshMs, imagePath.string());
        return false;
    }
    _logger->info("Displayed image: {} (panel refresh {} ms)", imagePath.string(), refreshMs);
    return true;
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
    return false;
#endif
}

void DisplayManager::sleep()
{
    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display sleep");
        return;
    }

    _hwSleep();
}

bool DisplayManager::_hwInit()
{
#ifdef EPD_HARDWARE_ENABLED
    if (!_initialized)
    {
        if (DEV_Module_Init() != 0)
        {
            _logger->error("DEV_Module_Init failed");
            return false;
        }
        _initialized = true;
    }
    const auto t0 = std::chrono::steady_clock::now();
    if (EPD_7IN5_V2_Init() != 0)
    {
        _logger->error("Panel init failed (power-on BUSY timeout)");
        return false;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _logger->debug("Panel init done (reset + power-on, {} ms)", ms);
    return true;
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
    return false;
#endif
}

void DisplayManager::_hwSleep()
{
#ifdef EPD_HARDWARE_ENABLED
    EPD_7IN5_V2_Sleep();
    _logger->debug("Panel powered down (deep sleep)");
#endif
}

void DisplayManager::_waitMinRefresh() const
{
    const auto elapsed = std::chrono::steady_clock::now() - _lastDisplayTime;

    if (const auto minRefresh = std::chrono::seconds(_cfg.display.minRefreshSecs); elapsed < minRefresh)
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(minRefresh - elapsed);
        _logger->info("Waiting {}s for minimum refresh interval", remaining.count());
        std::this_thread::sleep_for(minRefresh - elapsed);
    }
}

std::vector<uint8_t> DisplayManager::_prepareImageBuffer(
    const std::filesystem::path& imagePath) const
{
    int w, h, channels;
    auto* data = stbi_load(imagePath.c_str(), &w, &h, &channels, 3);
    if (!data)
    {
        _logger->error("Failed to load image: {} ({})",
                       imagePath.string(), stbi_failure_reason());
        return {};
    }

    // Resize to display dimensions
    std::vector<uint8_t> resized(WIDTH * HEIGHT * 3);
    stbir_resize_uint8_linear(
        data, w, h, w * 3,
        resized.data(), WIDTH, HEIGHT, WIDTH * 3,
        STBIR_RGB);
    stbi_image_free(data);

    // Convert to grayscale
    std::vector<float> gray(WIDTH * HEIGHT);
    for (int i = 0; i < WIDTH * HEIGHT; ++i)
    {
        const float r = resized[i * 3 + 0];
        const float g = resized[i * 3 + 1];
        const float b = resized[i * 3 + 2];
        gray[i] = 0.299f * r + 0.587f * g + 0.114f * b;
    }

    // Floyd-Steinberg dithering
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            const int idx = y * WIDTH + x;
            const float oldPixel = gray[idx];
            const float newPixel = (oldPixel > 127.5f) ? 255.0f : 0.0f;
            gray[idx] = newPixel;
            float error = oldPixel - newPixel;

            auto distribute = [&](const int dx, const int dy, const float factor)
            {
                const int ny = y + dy;
                if (const int nx = x + dx; nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT)
                    gray[ny * WIDTH + nx] += error * factor;
            };
            distribute(1, 0, 7.0f / 16.0f);
            distribute(-1, 1, 3.0f / 16.0f);
            distribute(0, 1, 5.0f / 16.0f);
            distribute(1, 1, 1.0f / 16.0f);
        }
    }

    // Pack into 1-bit buffer (MSB first, white=1 black=0)
    std::vector<uint8_t> buffer(BUFFER_SIZE, 0x00);
    for (int i = 0; i < WIDTH * HEIGHT; ++i)
    {
        if (gray[i] > 127.5f)
            buffer[i / 8] |= (0x80 >> (i % 8));
    }

    _logger->debug("Image buffer prepared: {} bytes", buffer.size());
    return buffer;
}
