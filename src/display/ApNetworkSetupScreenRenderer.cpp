#include "display/ApNetworkSetupScreenRenderer.hpp"
#include "display/QrLayout.hpp"
#include <spdlog/spdlog.h>

namespace
{
const std::shared_ptr<spdlog::logger>& logger()
{
    static const auto instance = spdlog::default_logger()->clone("ApNetworkSetupScreen");
    return instance;
}
} // namespace

std::string ApNetworkSetupScreenRenderer::wifiQrPayload(const std::string& ssid, const std::string& password)
{
    auto escape = [](const std::string& s)
    {
        std::string out;
        for (const char c : s)
        {
            if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"')
                out += '\\';
            out += c;
        }
        return out;
    };
    if (password.empty())
        return "WIFI:T:nopass;S:" + escape(ssid) + ";;";
    return "WIFI:T:WPA;S:" + escape(ssid) + ";P:" + escape(password) + ";;";
}

std::vector<uint8_t> ApNetworkSetupScreenRenderer::render(const std::string& ssid, const std::string& password,
                                                          const std::string& url, const std::string& fontPath)
{
    panel_text::Canvas canvas;
    qr_layout::drawQr(canvas, wifiQrPayload(ssid, password));

    if (panel_text::Font font; font.load(fontPath))
    {
        const std::string join = password.empty()
                                     ? "mit WLAN " + ssid + " verbinden"
                                     : "mit WLAN " + ssid + " verbinden (Passwort " + password + ")";
        qr_layout::drawText(canvas, font, {
                     "Rahmen einrichten",
                     "1. QR-Code scannen oder " + join,
                     "2. Dashboard öffnet sich automatisch, sonst " + url + " aufrufen und mit dem Rahmen-Passwort anmelden"
                 });
    }
    else
        logger()->error("Failed to load font {}, setup screen shows the QR code only", fontPath);

    return canvas.take();
}
