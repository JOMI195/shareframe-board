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

namespace
{
int64_t nowUnix()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

DisplayManager::DisplayManager(const AppConfig& cfg, DisplayMetricsRepository& metrics)
    : _cfg(cfg), _metrics(metrics), _logger(spdlog::default_logger()->clone("Display"))
{
    _lastDisplayTime.store(std::chrono::steady_clock::now()
        - std::chrono::seconds(_cfg.display.minRefreshSecs));
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
    std::lock_guard lock(_hwMutex);

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

    _clearHw();
    if (!_cfg.display.mockDisplay)
        std::this_thread::sleep_for(std::chrono::seconds(FIRST_START_SETTLE_SECS));
    if (!_displayImageHw(std::filesystem::path(_cfg.display.loadingImagePath) /
        "logo-frame-loading-shareframe.jpg"))
        _logger->error("Failed to display loading image");
}

void DisplayManager::clear()
{
    std::lock_guard lock(_hwMutex);
    _waitMinRefresh();
    _clearHw();
}

void DisplayManager::clearForShutdown()
{
    std::lock_guard lock(_hwMutex);
    if (_isCleared.load())
    {
        _logger->info("Display already cleared, skipping shutdown clear");
        return;
    }
    // No _waitMinRefresh(): shutdown is time-boxed and must not block.
    _logger->info("Clearing display before shutdown");
    _clearHw();
}

void DisplayManager::requestShutdown()
{
    // Set under _waitMtx to avoid a lost wakeup vs. a waiter about to block.
    {
        std::lock_guard lk(_waitMtx);
        _shuttingDown.store(true);
    }
    _waitCv.notify_all(); // wake any in-progress _waitMinRefresh()
}

void DisplayManager::_clearHw()
{
    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display clear");
        _lastDisplayTime.store(std::chrono::steady_clock::now());
        _isCleared = true;
        return;
    }

#ifdef EPD_HARDWARE_ENABLED
    if (!_hwInit())
        return;

    const auto t0 = std::chrono::steady_clock::now();
    const UBYTE rc = EPD_7IN5_V2_Clear();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _lastDisplayTime.store(std::chrono::steady_clock::now());
    _hwSleep();
    if (rc != 0)
    {
        _metrics.increment("epd_refresh_fail_total");
        ++_consecutiveFailures;
        _logger->error("Display clear FAILED (BUSY timeout, {} ms)", ms);
    }
    else
    {
        _metrics.increment("epd_refresh_total");
        _metrics.increment("epd_clear_total");
        _metrics.increment("epd_busy_ms_total", ms);
        _metrics.set("epd_last_refresh_ms", ms);
        _metrics.set("epd_last_refresh_at", nowUnix());
        _metrics.setIfUnset("epd_first_use_at", nowUnix());
        _consecutiveFailures = 0;
        _isCleared = true;
        _logger->info("Display cleared (panel refresh {} ms)", ms);
    }
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
#endif
}

bool DisplayManager::displayImage(const std::filesystem::path& imagePath)
{
    std::lock_guard lock(_hwMutex);
    _waitMinRefresh();
    return _displayImageHw(imagePath);
}

bool DisplayManager::_displayImageHw(const std::filesystem::path& imagePath)
{
    // Shutdown may have interrupted the wait; don't start a new refresh now.
    if (_shuttingDown.load())
    {
        _logger->info("Shutdown in progress, skipping image display");
        return false;
    }

    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display image: {}", imagePath.string());
        _lastDisplayTime.store(std::chrono::steady_clock::now());
        _isCleared = false;
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
    _lastDisplayTime.store(std::chrono::steady_clock::now());
    _hwSleep();
    if (rc != 0)
    {
        _metrics.increment("epd_refresh_fail_total");
        ++_consecutiveFailures;
        _logger->error("Panel refresh FAILED (BUSY timeout, {} ms): {}", refreshMs, imagePath.string());
        return false;
    }
    _metrics.increment("epd_refresh_total");
    _metrics.increment("epd_image_refresh_total");
    _metrics.increment("epd_busy_ms_total", refreshMs);
    _metrics.set("epd_last_refresh_ms", refreshMs);
    _metrics.set("epd_last_refresh_at", nowUnix());
    _metrics.setIfUnset("epd_first_use_at", nowUnix());
    _consecutiveFailures = 0;
    _isCleared = false;
    _logger->info("Displayed image: {} (panel refresh {} ms)", imagePath.string(), refreshMs);
    return true;
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
    return false;
#endif
}

void DisplayManager::sleep()
{
    std::lock_guard lock(_hwMutex);
    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display sleep");
        return;
    }

    _hwSleep();
}

void DisplayManager::_hwSleep()
{
#ifdef EPD_HARDWARE_ENABLED
    EPD_7IN5_V2_Sleep();
    _logger->debug("Panel powered down (deep sleep)");
#endif
}

nlohmann::json DisplayManager::healthSnapshot() const
{
    // Deliberately no _hwMutex (held for the whole multi-second refresh cycle):
    // metric reads use the repository mutex, the failure streak is atomic.
    const auto metrics = _metrics.all();
    const int failures = _consecutiveFailures.load(std::memory_order_relaxed);

    nlohmann::json j;
    for (const auto& [key, value] : metrics)
        j[key] = value;

    const auto it = metrics.find("epd_refresh_total");
    const int64_t refreshes = (it != metrics.end()) ? it->second : 0;

    // No panel sensor: health derives from the consecutive-failure streak.
    j["consecutive_failures"] = failures;
    j["rated_refreshes"] = RATED_REFRESHES;
    j["wear_percent"] = static_cast<double>(refreshes) / RATED_REFRESHES * 100.0;
    j["health"] = (failures == 0)  ? "ok"
                  : (failures < 3) ? "degraded"
                                   : "failed";
    return j;
}

bool DisplayManager::_hwInit()
{
#ifdef EPD_HARDWARE_ENABLED
    if (!_initialized)
    {
        if (DEV_Module_Init() != 0)
        {
            _metrics.increment("epd_poweron_fail_total");
            ++_consecutiveFailures;
            _logger->error("DEV_Module_Init failed");
            return false;
        }
        _initialized = true;
    }
    const auto t0 = std::chrono::steady_clock::now();
    if (EPD_7IN5_V2_Init() != 0)
    {
        _metrics.increment("epd_poweron_fail_total");
        ++_consecutiveFailures;
        _logger->error("Panel init failed (power-on BUSY timeout)");
        return false;
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _metrics.increment("epd_poweron_total");
    _consecutiveFailures = 0;
    _logger->debug("Panel init done (reset + power-on, {} ms)", ms);
    return true;
#else
    _logger->warn("EPD hardware not available (built without ENABLE_EPD_HARDWARE)");
    return false;
#endif
}

int DisplayManager::secondsUntilRefreshReady() const
{
    const auto elapsed = std::chrono::steady_clock::now() - _lastDisplayTime.load();
    const auto minRefresh = std::chrono::seconds(_cfg.display.minRefreshSecs);
    if (elapsed >= minRefresh)
        return 0;
    return static_cast<int>(
        std::chrono::ceil<std::chrono::seconds>(minRefresh - elapsed).count());
}

void DisplayManager::_waitMinRefresh() const
{
    const auto elapsed = std::chrono::steady_clock::now() - _lastDisplayTime.load();
    const auto minRefresh = std::chrono::seconds(_cfg.display.minRefreshSecs);
    if (elapsed >= minRefresh)
        return;

    const auto remaining = minRefresh - elapsed;
    _logger->info("Waiting {}s for minimum refresh interval",
                  std::chrono::duration_cast<std::chrono::seconds>(remaining).count());
    // Interruptible (requestShutdown wakes us): called with _hwMutex held, so a
    // plain sleep would pin the bus and stall shutdown.
    std::unique_lock lk(_waitMtx);
    _waitCv.wait_for(lk, remaining, [this] { return _shuttingDown.load(); });
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
