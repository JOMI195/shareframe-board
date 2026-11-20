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

    /// The German instructions beside the QR code; the setup code is the way to sign in.
    [[nodiscard]] static std::vector<std::string> paragraphs(const std::string& ssid, const std::string& password,
                                                             const std::string& url, const std::string& code,
                                                             const std::string& failure = {});

    /// The full 1-bit panel buffer (1 = white). `codeLast`: the last paragraph is the setup code, set large.
    [[nodiscard]] static std::vector<uint8_t> render(const std::string& ssid, const std::string& password,
                                                     const std::vector<std::string>& paragraphs,
                                                     const std::string& fontPath, bool codeLast = false);
};
