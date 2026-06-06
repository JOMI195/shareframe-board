#pragma once
#include <optional>
#include <string>

enum class WsMessageType
{
    // inbound (server → client)
    Picture,
    ClearImages,
    ClearDisplay,
    // outbound (client → server)
    CheckSentImagesExpiry,
    CheckMissingImages,
};

inline std::string wsMessageTypeToString(WsMessageType t)
{
    switch (t)
    {
    case WsMessageType::Picture:               return "picture";
    case WsMessageType::ClearImages:           return "clear_specific_sent_images";
    case WsMessageType::ClearDisplay:          return "clear_display";
    case WsMessageType::CheckSentImagesExpiry: return "check_sent_images_expiry";
    case WsMessageType::CheckMissingImages:    return "check_missing_images";
    }
    return "unknown";
}

inline std::optional<WsMessageType> wsMessageTypeFromString(const std::string& s)
{
    if (s == "picture")                      return WsMessageType::Picture;
    if (s == "clear_specific_sent_images")   return WsMessageType::ClearImages;
    if (s == "clear_display")                return WsMessageType::ClearDisplay;
    if (s == "check_sent_images_expiry")     return WsMessageType::CheckSentImagesExpiry;
    if (s == "check_missing_images")         return WsMessageType::CheckMissingImages;
    return std::nullopt;
}
