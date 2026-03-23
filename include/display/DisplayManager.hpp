#pragma once
#include "config/AppConfig.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

class DisplayManager
{
public:
    explicit DisplayManager(const AppConfig& cfg);
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

    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 480;
    static constexpr int BUFFER_SIZE = WIDTH * HEIGHT / 8;

private:
    [[nodiscard]] bool _hwInit();
    void _hwSleep();
    void _waitMinRefresh();
    [[nodiscard]] std::vector<uint8_t> _prepareImageBuffer(
        const std::filesystem::path& imagePath) const;

    const AppConfig& _cfg;
    std::shared_ptr<spdlog::logger> _logger;
    bool _initialized = false;
    std::chrono::steady_clock::time_point _lastDisplayTime{};
};
