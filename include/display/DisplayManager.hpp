#pragma once
#include "config/AppConfig.hpp"
#include "display/PanelState.hpp"
#include "display/PartialRefresh.hpp"
#include "fsm/Machine.hpp"
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
#include <stop_token>
#include <vector>

enum class RefreshPolicy
{
    RespectMinInterval,
    Immediate
};

/// What the panel shows and the source last painted; seq 0 before the first refresh.
struct ScreenSnapshot
{
    PanelContent state = PanelContent::Unknown;
    std::string source;
    int64_t seq = 0;
    int64_t updatedAt = 0;
};

class DisplayManager
{
public:
    DisplayManager(const AppConfig& cfg, DisplayMetricsRepository& metrics);
    ~DisplayManager();

    /// Power the panel up and clear it (DEV_Module_Init + EPD_Init + clear + settle);
    /// paints no content.
    void init();

    /// Clear the display to white (init → clear → sleep cycle). A stop request
    /// cancels a clear still waiting for the min refresh interval.
    void clear(std::stop_token st = {});

    /// Clear before shutdown if the panel still shows content; skips the
    /// min-refresh wait (shutdown can't block) and no-ops if already cleared.
    void clearForShutdown();

    /// Takes the place of clearForShutdown() when the panel should keep a screen
    /// through the reboot; skips the min-refresh wait for the same reason.
    bool paintForShutdown(const std::vector<uint8_t>& buffer, const std::string& source);

    /// Signal shutdown: wakes the min-refresh wait and makes pending displays bail,
    /// so tasks unblock within the timeout-kill window instead of holding the lock.
    void requestShutdown();

    /// Paint a ready 1-bit panel buffer (init → display → sleep cycle); `source`
    /// names the content for logs and the mock panel. A stop request cancels a
    /// paint still waiting for the min refresh interval. `text` is what the screen
    /// says: only the mock panel records it, so tests read it like a person would.
    [[nodiscard]] bool displayBuffer(const std::vector<uint8_t>& buffer, const std::string& source,
                                     std::stop_token st = {},
                                     RefreshPolicy policy = RefreshPolicy::RespectMinInterval,
                                     const std::vector<std::string>& text = {});

    /// Like displayBuffer(), but refreshes only the window that differs from what the panel
    /// holds when that is small; nothing at all when it holds the buffer already.
    [[nodiscard]] bool displayChanges(const std::vector<uint8_t>& buffer, const std::string& source,
                                      std::stop_token st = {},
                                      RefreshPolicy policy = RefreshPolicy::RespectMinInterval,
                                      const std::vector<std::string>& text = {});

    [[nodiscard]] bool isCleared() const { return _content.load() == PanelContent::Blank; }

    /// Seconds until the next refresh is allowed (0 if ready). Lock-free so a
    /// caller can wait it out and re-decide what to paint with fresh state.
    [[nodiscard]] int secondsUntilRefreshReady() const;

    /// Put the display into deep sleep mode.
    void sleep();

    /// Wear counters + derived health as JSON for the IPC query. Lock-free w.r.t.
    /// the hardware mutex so it answers even during a multi-second refresh.
    [[nodiscard]] nlohmann::json healthSnapshot() const;

    /// Lock-free w.r.t. the hardware mutex, like healthSnapshot().
    [[nodiscard]] ScreenSnapshot screenSnapshot() const;

    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 480;
    static constexpr int BUFFER_SIZE = WIDTH * HEIGHT / 8;
    static constexpr int FIRST_START_SETTLE_SECS = 5;
    /// Manufacturer full-refresh endurance estimate (UC8179 / Waveshare 7.5"V2).
    static constexpr int64_t RATED_REFRESHES = 1'000'000;

private:
    [[nodiscard]] bool _hwInit(bool partial = false);
    /// Call under the hardware lock; false when the paint should not happen.
    [[nodiscard]] bool _readyToPaint(const std::string& source, const std::stop_token& st, RefreshPolicy policy);
    void _hwSleep();
    void _waitMinRefresh(const std::stop_token& st = {}) const;
    void _clearHw();
    [[nodiscard]] bool _sendBufferHw(const std::vector<uint8_t>& buffer, const std::string& source,
                                     const std::vector<std::string>& text = {});
    [[nodiscard]] bool _sendPartialHw(const std::vector<uint8_t>& buffer, const partial_refresh::Window& window,
                                      const std::string& source, const std::vector<std::string>& text);
    void _writeMockPanel(const std::vector<uint8_t>& buffer, PanelContent content, const std::string& source,
                         const std::vector<std::string>& text = {}, bool partial = false);
    /// Call after a refresh that succeeded.
    void _recordScreen(PanelContent content, const std::string& source);

    const AppConfig& _cfg;
    DisplayMetricsRepository& _metrics;
    std::shared_ptr<spdlog::logger> _logger;
    fsm::Machine<PanelPower> _power;
    /// In-memory since boot; atomic so healthSnapshot() reads it without the
    /// hw mutex. Mutated only by the hw threads (already under _hwMutex).
    std::atomic<int> _consecutiveFailures{0};
    /// Written under the hardware lock; atomic so the tasks read it without.
    /// In-memory (boot always clears).
    std::atomic<PanelContent> _content{PanelContent::Unknown};
    /// Atomic so it reads lock-free during a multi-second refresh; mutated only
    /// by the hw threads.
    std::atomic<std::chrono::steady_clock::time_point> _lastDisplayTime{};
    std::mutex _hwMutex; ///< serializes all EPD hardware access (init/clear/display/sleep)
    /// What the panel holds, empty when unknown; under the hardware lock.
    std::vector<uint8_t> _shown;
    int _partialsInARow = 0;
    int64_t _mockSeq = 0;
    mutable std::mutex _screenMtx;
    ScreenSnapshot _screen;

    /// Set by requestShutdown(): makes the min-refresh wait return early so
    /// shutdown doesn't block.
    std::atomic<bool> _shuttingDown{false};
    mutable std::mutex _waitMtx;             ///< guards the interruptible min-refresh wait
    mutable std::condition_variable _waitCv; ///< woken by requestShutdown()
};
