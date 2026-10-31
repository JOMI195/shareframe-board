#include "display/RegistrationScreenRenderer.hpp"
#include "display/QrLayout.hpp"
#include <spdlog/spdlog.h>

std::string RegistrationScreenRenderer::registerUrl(const std::string& baseUrl, const std::string& registerPath,
                                                    const std::string& serial)
{
    return baseUrl + registerPath + serial;
}

std::vector<uint8_t> RegistrationScreenRenderer::render(const std::string& url, const std::string& host,
                                                        const std::string& serial, const std::string& fontPath)
{
    panel_text::Canvas canvas;
    qr_layout::drawQr(canvas, url);

    if (panel_text::Font font; font.load(fontPath))
        qr_layout::drawText(canvas, font, {
                                "Rahmen registrieren",
                                "1. QR-Code scannen und bei Shareframe anmelden",
                                "2. Oder auf " + host + " unter Bilderrahmen hinzufügen: " + serial
                            });
    else
        spdlog::error("Failed to load font {}, registration screen shows the QR code only", fontPath);

    return canvas.take();
}
