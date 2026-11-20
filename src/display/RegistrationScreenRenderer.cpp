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

std::vector<std::string> RegistrationScreenRenderer::paragraphs(const std::string& host, const std::string& serial,
                                                                const std::string& claimCode,
                                                                const std::string& hostname)
{
    std::vector<std::string> text{
        "Rahmen hinzufügen",
        "1. QR-Code scannen und bei ShareFrame anmelden",
        "2. Oder auf " + host + " unter Bilderrahmen hinzufügen diesen Code eingeben:",
        claimCode.empty() ? "Code wird geladen …" : formatClaimCode(claimCode),
    };
    if (!hostname.empty())
        text.push_back("Einstellungen: http://" + hostname + ".local");
    text.push_back("Seriennummer: " + serial);
    return text;
}

std::vector<uint8_t> RegistrationScreenRenderer::render(const std::string& url,
                                                        const std::vector<std::string>& paragraphs,
                                                        const std::string& fontPath)
{
    panel_text::Canvas canvas;
    qr_layout::drawQr(canvas, url);

    if (panel_text::Font font; font.load(fontPath))
        qr_layout::drawText(canvas, font, paragraphs);
    else
        spdlog::error("Failed to load font {}, registration screen shows the QR code only", fontPath);

    return canvas.take();
}
