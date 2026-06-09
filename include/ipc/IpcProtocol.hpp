#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

enum class IpcMessageType
{
    SkipImage,
    UpdateDisplayInterval,
    GetDisplayInterval,
    ClearDisplay,
    SetSlideshowActive,
    GetSlideshowActive,
    GetSecondsUntilNext,
    GetHealth,
};

struct IpcMessage
{
    IpcMessageType type;
    nlohmann::json data;
};

inline std::string ipcMessageTypeToString(IpcMessageType t)
{
    switch (t)
    {
    case IpcMessageType::SkipImage:              return "skip_image";
    case IpcMessageType::UpdateDisplayInterval:   return "update_display_interval";
    case IpcMessageType::GetDisplayInterval:      return "get_display_interval";
    case IpcMessageType::ClearDisplay:            return "clear_display";
    case IpcMessageType::SetSlideshowActive:      return "set_slideshow_active";
    case IpcMessageType::GetSlideshowActive:      return "get_slideshow_active";
    case IpcMessageType::GetSecondsUntilNext:     return "get_seconds_until_next";
    case IpcMessageType::GetHealth:               return "get_health";
    }
    return "unknown";
}

inline std::optional<IpcMessageType> ipcMessageTypeFromString(const std::string& s)
{
    if (s == "skip_image")               return IpcMessageType::SkipImage;
    if (s == "update_display_interval")  return IpcMessageType::UpdateDisplayInterval;
    if (s == "get_display_interval")     return IpcMessageType::GetDisplayInterval;
    if (s == "clear_display")           return IpcMessageType::ClearDisplay;
    if (s == "set_slideshow_active")    return IpcMessageType::SetSlideshowActive;
    if (s == "get_slideshow_active")    return IpcMessageType::GetSlideshowActive;
    if (s == "get_seconds_until_next")  return IpcMessageType::GetSecondsUntilNext;
    if (s == "get_health")              return IpcMessageType::GetHealth;
    return std::nullopt;
}

inline std::string toJson(const IpcMessage& msg)
{
    nlohmann::json j;
    j["type"] = ipcMessageTypeToString(msg.type);
    if (!msg.data.is_null())
        j["data"] = msg.data;
    return j.dump() + "\n";
}

inline std::optional<IpcMessage> parseIpcMessage(const std::string& line)
{
    try
    {
        auto j = nlohmann::json::parse(line);
        auto typeStr = j.at("type").get<std::string>();
        auto type = ipcMessageTypeFromString(typeStr);
        if (!type)
            return std::nullopt;
        nlohmann::json data;
        if (j.contains("data"))
            data = j["data"];
        return IpcMessage{*type, std::move(data)};
    }
    catch (...)
    {
        return std::nullopt;
    }
}

inline std::string toJsonResponse(const nlohmann::json& data)
{
    nlohmann::json j;
    j["type"] = "response";
    j["data"] = data;
    return j.dump() + "\n";
}
