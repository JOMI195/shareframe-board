#pragma once
#include <optional>
#include <string>

enum class ImageQuality
{
    Mono,
    Gray4
};

inline std::string toString(const ImageQuality quality)
{
    return quality == ImageQuality::Gray4 ? "gray4" : "mono";
}

inline std::optional<ImageQuality> imageQualityFromString(const std::string& s)
{
    if (s == "mono")  return ImageQuality::Mono;
    if (s == "gray4") return ImageQuality::Gray4;
    return std::nullopt;
}
