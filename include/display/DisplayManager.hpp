#pragma once
#include "config/AppConfig.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include <atomic>
#include <chrono>
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

    /// Load image from path, resize to 800x480, convert to 1-bit,
    /// and display on the e-paper (init → display → sleep cycle).
    [[nodiscard]] bool displayImage(const std::filesystem::path& imagePath);

    /// Put the display into deep sleep mode.
    void sleep();

    /// Persisted wear counters + derived health (status, wear%, consecutive
    /// failures) as JSON, for the get_display_stats IPC query. Lock-free w.r.t.
    /// the hardware mutex: it must answer the IPC REP thread immediately even
    /// while a multi-second panel refresh holds _hwMutex.
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
    /// In-memory since boot; drives health status. Atomic so healthSnapshot()
    /// can read it from the REP thread without taking _hwMutex (which a refresh
    /// holds for seconds). Mutated only by the hw threads, already under _hwMutex.
    std::atomic<int> _consecutiveFailures{0};
    std::chrono::steady_clock::time_point _lastDisplayTime{};
    std::mutex _hwMutex; ///< serializes all EPD hardware access (init/clear/display/sleep)
};
