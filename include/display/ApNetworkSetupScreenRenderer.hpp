#pragma once
#include "display/DisplayManager.hpp"
#include "util/NonInstantiable.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// The Wi-Fi setup screen shown while the frame hosts its access point: a QR code to
/// join the AP and short German instructions, drawn only where the passepartout
/// leaves the panel visible.
class ApNetworkSetupScreenRenderer : NonInstantiable
{
public:
    struct Rect
    {
        int x0, y0, x1, y1;
    };

    /// About 140×90 mm of the 163.2×97.92 mm panel show through the passepartout;
    /// 5 mm more per side absorb a frame that isn't mounted dead centre.
    static constexpr Rect SAFE_AREA = []
    {
        constexpr double pxPerMm = DisplayManager::WIDTH / 163.2;
        constexpr double visibleW = 140.0 * pxPerMm;
        constexpr double visibleH = 90.0 * pxPerMm;
        constexpr double margin = 5.0 * pxPerMm;
        const int x0 = static_cast<int>((DisplayManager::WIDTH - visibleW) / 2 + margin) + 1;
        const int y0 = static_cast<int>((DisplayManager::HEIGHT - visibleH) / 2 + margin) + 1;
        return Rect{x0, y0, DisplayManager::WIDTH - x0, DisplayManager::HEIGHT - y0};
    }();

    /// Joins the network when scanned; an empty password is an open network.
    [[nodiscard]] static std::string wifiQrPayload(const std::string& ssid, const std::string& password);

    /// The full 1-bit panel buffer (1 = white).
    [[nodiscard]] static std::vector<uint8_t> render(const std::string& ssid, const std::string& password,
                                                     const std::string& url, const std::string& fontPath);
};
