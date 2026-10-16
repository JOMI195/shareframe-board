#pragma once
#include "util/NonInstantiable.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/// The logo picture with a bold status line and an optional detail line centred
/// beneath it, where the old loading picture had its text.
class LogoScreenRenderer : NonInstantiable
{
public:
    /// The logo picture in display.loading_image_path.
    static constexpr auto IMAGE = "logo-frame-generic-shareframe.jpg";

    /// "v1.2.0" for a release; a dev build shows its short sha instead.
    [[nodiscard]] static std::string versionLabel(const std::string& version, const std::string& sha);

    /// The full 1-bit panel buffer; an unreadable picture leaves the text on white.
    [[nodiscard]] static std::vector<uint8_t> render(const std::filesystem::path& imagePath, const std::string& title,
                                                     const std::string& detail, const std::string& boldFontPath,
                                                     const std::string& regularFontPath);
};
