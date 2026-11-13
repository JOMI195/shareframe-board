#include "display/RegistrationScreenRenderer.hpp"
#include "display/QrLayout.hpp"
#include <spdlog/spdlog.h>

std::string RegistrationScreenRenderer::registerUrl(const std::string& baseUrl, const std::string& registerPath,
                                                    const std::string& claimCode)
{
    return baseUrl + registerPath + claimCode;
}

std::string RegistrationScreenRenderer::formatClaimCode(const std::string& claimCode)
{
    std::string grouped;
    for (size_t i = 0; i < claimCode.size(); ++i)
    {
        if (i > 0 && i % 4 == 0)
            grouped += '-';
        grouped += claimCode[i];
    }
    return grouped;
}

std::vector<uint8_t> RegistrationScreenRenderer::render(const std::string& url, const std::string& host,
                                                        const std::string& serial, const std::string& claimCode,
                                                        const std::string& fontPath)
{
    panel_text::Canvas canvas;
    qr_layout::drawQr(canvas, url);

    std::vector<std::string> paragraphs{
        "Rahmen hinzufügen",
        "1. QR-Code scannen und bei ShareFrame anmelden",
        "2. Oder auf " + host + " unter Bilderrahmen hinzufügen diesen Code eingeben:",
        claimCode.empty() ? "Code wird geladen …" : formatClaimCode(claimCode),
        "Seriennummer: " + serial,
    };

    if (panel_text::Font font; font.load(fontPath))
        qr_layout::drawText(canvas, font, paragraphs);
    else
        spdlog::error("Failed to load font {}, registration screen shows the QR code only", fontPath);

    return canvas.take();
}
