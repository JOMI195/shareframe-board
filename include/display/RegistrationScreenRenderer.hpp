#pragma once
#include "util/NonInstantiable.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// Shown while the server refuses the frame because nobody owns it yet: a QR code that
/// opens the website's register dialog with the claim code filled in, and the code as text.
/// The code alone claims the frame; the serial is printed for support only.
class RegistrationScreenRenderer : NonInstantiable
{
public:
    [[nodiscard]] static std::string registerUrl(const std::string& baseUrl, const std::string& registerPath,
                                                 const std::string& claimCode);

    /// Groups the code into blocks of four, the way the website's register dialog shows it.
    [[nodiscard]] static std::string formatClaimCode(const std::string& claimCode);

    /// The text beside the QR code; `hostname` gives the dashboard's address, none without it.
    [[nodiscard]] static std::vector<std::string> paragraphs(const std::string& host, const std::string& serial,
                                                             const std::string& claimCode, const std::string& hostname);

    /// The full 1-bit panel buffer (1 = white).
    [[nodiscard]] static std::vector<uint8_t> render(const std::string& url, const std::vector<std::string>& paragraphs,
                                                     const std::string& fontPath);
};
