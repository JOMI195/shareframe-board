#include "display/DisplayManager.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
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

// The mock panel stays 1-bit: light grays become white, dark grays black.
std::vector<uint8_t> toMono(const std::vector<uint8_t>& buffer)
{
    if (buffer.size() != DisplayManager::GRAY4_BUFFER_SIZE)
        return buffer;
    std::vector<uint8_t> mono(DisplayManager::BUFFER_SIZE, 0x00);
    for (int i = 0; i < DisplayManager::WIDTH * DisplayManager::HEIGHT; ++i)
    {
        if ((buffer[i / 4] >> (6 - 2 * (i % 4))) & 0b10)
            mono[i / 8] |= (0x80 >> (i % 8));
    }
    return mono;
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
        if (_panelPowered)
            _hwSleep();
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
        if (!_cfg.display.mockOutputDir.empty())
            _writeMockPanel(std::vector<uint8_t>(BUFFER_SIZE, 0xFF), "cleared", {});
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

bool DisplayManager::displayBuffer(const std::vector<uint8_t>& buffer, const std::string& source,
                                   const std::stop_token st, const RefreshPolicy policy)
{
    std::lock_guard lock(_hwMutex);
    if (policy == RefreshPolicy::Immediate)
    {
        if (const int wait = secondsUntilRefreshReady(); wait > 0)
            _logger->info("Immediate refresh for {} (min interval bypassed, {}s early)", source, wait);
    }
    else
        _waitMinRefresh(st);

    if (st.stop_requested())
    {
        _logger->debug("Display of {} cancelled", source);
        return false;
    }
    return _sendBufferHw(buffer, source);
}

bool DisplayManager::_sendBufferHw(const std::vector<uint8_t>& buffer, const std::string& source)
{
    // Shutdown may have interrupted the wait; don't start a new refresh now.
    if (_shuttingDown.load())
    {
        _logger->info("Shutdown in progress, skipping display of {}", source);
        return false;
    }

    if (buffer.size() != BUFFER_SIZE && buffer.size() != GRAY4_BUFFER_SIZE)
    {
        _logger->error("Refusing to display {}: buffer has {} bytes, expected {} or {}",
                       source, buffer.size(), BUFFER_SIZE, GRAY4_BUFFER_SIZE);
        return false;
    }
    const bool gray4 = buffer.size() == GRAY4_BUFFER_SIZE;

    if (_cfg.display.mockDisplay)
    {
        _logger->info("[mock] Display: {}", source);
        if (!_cfg.display.mockOutputDir.empty())
            _writeMockPanel(buffer, "image", source);
        _lastDisplayTime.store(std::chrono::steady_clock::now());
        _isCleared = false;
        return true;
    }

#ifdef EPD_HARDWARE_ENABLED
    if (!_hwInit(gray4))
        return false;

    _logger->debug("Sending buffer to panel ({} bytes) and refreshing", buffer.size());
    const auto t0 = std::chrono::steady_clock::now();
    UBYTE rc = 0;
    if (gray4)
        EPD_7IN5_V2_Display_4Gray(buffer.data());
    else
        rc = EPD_7IN5_V2_Display(const_cast<UBYTE*>(buffer.data()));
    const auto refreshMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    _lastDisplayTime.store(std::chrono::steady_clock::now());
    _hwSleep();
    if (rc != 0)
    {
        _metrics.increment("epd_refresh_fail_total");
        ++_consecutiveFailures;
        _logger->error("Panel refresh FAILED (BUSY timeout, {} ms): {}", refreshMs, source);
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
    _logger->info("Displayed {} ({}, panel refresh {} ms)", source, gray4 ? "4-gray" : "1-bit", refreshMs);
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
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_PWR_PIN, 0);
    _panelPowered = false;
    _logger->debug("Panel deep sleep + power cut");
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

bool DisplayManager::_hwInit(const bool gray4)
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
    DEV_Digital_Write(EPD_PWR_PIN, 1);
    _panelPowered = true;
    const auto t0 = std::chrono::steady_clock::now();
    if ((gray4 ? EPD_7IN5_V2_Init_4Gray() : EPD_7IN5_V2_Init()) != 0)
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

void DisplayManager::_waitMinRefresh(const std::stop_token& st) const
{
    const auto elapsed = std::chrono::steady_clock::now() - _lastDisplayTime.load();
    const auto minRefresh = std::chrono::seconds(_cfg.display.minRefreshSecs);
    if (elapsed >= minRefresh)
        return;

    const auto remaining = minRefresh - elapsed;
    _logger->info("Waiting {}s for minimum refresh interval",
                  std::chrono::duration_cast<std::chrono::seconds>(remaining).count());
    // Interruptible (shutdown or the caller's stop wakes us): called with _hwMutex held, so a
    // plain sleep would pin the bus and stall shutdown.
    std::stop_callback onStop(st, [this]
    {
        std::lock_guard lk(_waitMtx);
        _waitCv.notify_all();
    });
    std::unique_lock lk(_waitMtx);
    _waitCv.wait_for(lk, remaining, [this, &st] { return _shuttingDown.load() || st.stop_requested(); });
}

void DisplayManager::_writeMockPanel(const std::vector<uint8_t>& buffer, const std::string& state,
                                     const std::string& source)
{
    const std::filesystem::path dir(_cfg.display.mockOutputDir);
    std::filesystem::create_directories(dir);

    // PBM stores 1 = black, the panel buffer 1 = white.
    {
        std::ofstream pbm(dir / "panel.pbm.tmp", std::ios::binary);
        pbm << "P4\n" << WIDTH << " " << HEIGHT << "\n";
        for (const uint8_t b : toMono(buffer))
            pbm.put(static_cast<char>(~b));
    }
    std::filesystem::rename(dir / "panel.pbm.tmp", dir / "panel.pbm");

    {
        std::ofstream meta(dir / "panel.json.tmp");
        meta << nlohmann::json{{"state", state}, {"source", source}, {"seq", ++_mockSeq},
                               {"gray4", buffer.size() == GRAY4_BUFFER_SIZE}}.dump();
    }
    std::filesystem::rename(dir / "panel.json.tmp", dir / "panel.json");

    _metrics.increment("epd_refresh_total");
    _metrics.increment(state == "image" ? "epd_image_refresh_total" : "epd_clear_total");
    _metrics.set("epd_last_refresh_at", nowUnix());
    _metrics.setIfUnset("epd_first_use_at", nowUnix());
}
