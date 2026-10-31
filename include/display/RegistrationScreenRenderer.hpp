#pragma once
#include "util/NonInstantiable.hpp"
#include <cstdint>
#include <string>
#include <vector>

/// Shown while the server refuses the frame because nobody owns it yet: a QR code that
/// opens the website's register dialog with the serial filled in, and the serial itself.
class RegistrationScreenRenderer : NonInstantiable
{
public:
    [[nodiscard]] static std::string registerUrl(const std::string& baseUrl, const std::string& registerPath,
                                                 const std::string& serial);

    /// The full 1-bit panel buffer (1 = white).
    [[nodiscard]] static std::vector<uint8_t> render(const std::string& url, const std::string& host,
                                                     const std::string& serial, const std::string& fontPath);
};
