#pragma once
#include "display/PanelText.hpp"
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
    static constexpr panel_text::Rect SAFE_AREA = panel_text::SAFE_AREA;

    /// Joins the network when scanned; an empty password is an open network.
    [[nodiscard]] static std::string wifiQrPayload(const std::string& ssid, const std::string& password);

    /// The full 1-bit panel buffer (1 = white).
    [[nodiscard]] static std::vector<uint8_t> render(const std::string& ssid, const std::string& password,
                                                     const std::string& url, const std::string& fontPath);
};
