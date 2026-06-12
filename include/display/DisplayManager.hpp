#pragma once
#include "config/AppConfig.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <vector>

class DisplayManager
{
public:
    DisplayManager(const AppConfig& cfg, DisplayMetricsRepository& metrics);
    ~DisplayManager();

    /// Initialize the e-paper hardware (DEV_Module_Init + EPD_Init).
    void init();

    /// Clear the display to white (init → clear → sleep cycle).
    void clear();

    /// Clear before shutdown if the panel still shows content; skips the
    /// min-refresh wait (shutdown can't block) and no-ops if already cleared.
    void clearForShutdown();

    /// Signal shutdown: wakes the min-refresh wait and makes pending displays bail,
    /// so tasks unblock within the timeout-kill window instead of holding the lock.
    void requestShutdown();

    /// Load image from path, resize to 800x480, convert to 1-bit,
    /// and display on the e-paper (init → display → sleep cycle).
    [[nodiscard]] bool displayImage(const std::filesystem::path& imagePath);

    /// Seconds until the next refresh is allowed (0 if ready). Lock-free so a
    /// caller can wait it out and re-decide what to paint with fresh state.
    [[nodiscard]] int secondsUntilRefreshReady() const;

    /// Put the display into deep sleep mode.
    void sleep();

    /// Wear counters + derived health as JSON for the IPC query. Lock-free w.r.t.
    /// the hardware mutex so it answers even during a multi-second refresh.
    [[nodiscard]] nlohmann::json healthSnapshot() const;

    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 480;
    static constexpr int BUFFER_SIZE = WIDTH * HEIGHT / 8;
    static constexpr int FIRST_START_SETTLE_SECS = 5;
    /// Manufacturer full-refresh endurance estimate (UC8179 / Waveshare 7.5"V2).
    static constexpr int64_t RATED_REFRESHES = 1'000'000;

private:
    [[nodiscard]] bool _hwInit();
    void _hwSleep();
    void _waitMinRefresh() const;
    void _clearHw();
    [[nodiscard]] bool _displayImageHw(const std::filesystem::path& imagePath);
    [[nodiscard]] std::vector<uint8_t> _prepareImageBuffer(
        const std::filesystem::path& imagePath) const;

    const AppConfig& _cfg;
    DisplayMetricsRepository& _metrics;
    std::shared_ptr<spdlog::logger> _logger;
    bool _initialized = false;
    /// In-memory since boot; atomic so healthSnapshot() reads it without the
    /// hw mutex. Mutated only by the hw threads (already under _hwMutex).
    std::atomic<int> _consecutiveFailures{0};
    /// True when the panel shows white. Written under _hwMutex; read on shutdown
    /// to decide if a final clear is needed. In-memory (boot always clears).
    std::atomic<bool> _isCleared{false};
    /// Atomic so it reads lock-free during a multi-second refresh; mutated only
    /// by the hw threads.
    std::atomic<std::chrono::steady_clock::time_point> _lastDisplayTime{};
    std::mutex _hwMutex; ///< serializes all EPD hardware access (init/clear/display/sleep)

    /// Set by requestShutdown(): makes the min-refresh wait return early so
    /// shutdown doesn't block.
    std::atomic<bool> _shuttingDown{false};
    mutable std::mutex _waitMtx;             ///< guards the interruptible min-refresh wait
    mutable std::condition_variable _waitCv; ///< woken by requestShutdown()
};
