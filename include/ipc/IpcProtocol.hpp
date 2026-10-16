#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

enum class IpcMessageType
{
    SkipImage,
    UpdateDisplayInterval,
    GetDisplayInterval,
    UpdateNightMode,
    GetNightMode,
    UpdatePlaybackOrder,
    GetPlaybackOrder,
    GetScreenInfo,
    ClearDisplay,
    SetSlideshowActive,
    GetSlideshowActive,
    GetSecondsUntilNext,
    GetHealth,
    GetDisplayStats,
    UpdateCheckLatest,
    UpdatePerform,
    UpdateStatus,
    UpdateHistory,
    ShowStatusScreen,
    HideStatusScreen,
    ShowImage,
    GetServerConnection,
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
    case IpcMessageType::UpdateNightMode:         return "update_night_mode";
    case IpcMessageType::GetNightMode:            return "get_night_mode";
    case IpcMessageType::UpdatePlaybackOrder:     return "update_playback_order";
    case IpcMessageType::GetPlaybackOrder:        return "get_playback_order";
    case IpcMessageType::GetScreenInfo:           return "get_screen_info";
    case IpcMessageType::ClearDisplay:            return "clear_display";
    case IpcMessageType::SetSlideshowActive:      return "set_slideshow_active";
    case IpcMessageType::GetSlideshowActive:      return "get_slideshow_active";
    case IpcMessageType::GetSecondsUntilNext:     return "get_seconds_until_next";
    case IpcMessageType::GetHealth:               return "get_health";
    case IpcMessageType::GetDisplayStats:         return "get_display_stats";
    case IpcMessageType::UpdateCheckLatest:       return "update_check_latest";
    case IpcMessageType::UpdatePerform:           return "update_perform";
    case IpcMessageType::UpdateStatus:            return "update_status";
    case IpcMessageType::UpdateHistory:           return "update_history";
    case IpcMessageType::ShowStatusScreen:        return "show_status_screen";
    case IpcMessageType::HideStatusScreen:        return "hide_status_screen";
    case IpcMessageType::ShowImage:               return "show_image";
    case IpcMessageType::GetServerConnection:     return "get_server_connection";
    }
    return "unknown";
}

inline std::optional<IpcMessageType> ipcMessageTypeFromString(const std::string& s)
{
    if (s == "skip_image")               return IpcMessageType::SkipImage;
    if (s == "update_display_interval")  return IpcMessageType::UpdateDisplayInterval;
    if (s == "get_display_interval")     return IpcMessageType::GetDisplayInterval;
    if (s == "update_night_mode")       return IpcMessageType::UpdateNightMode;
    if (s == "get_night_mode")          return IpcMessageType::GetNightMode;
    if (s == "update_playback_order")   return IpcMessageType::UpdatePlaybackOrder;
    if (s == "get_playback_order")      return IpcMessageType::GetPlaybackOrder;
    if (s == "get_screen_info")         return IpcMessageType::GetScreenInfo;
    if (s == "clear_display")           return IpcMessageType::ClearDisplay;
    if (s == "set_slideshow_active")    return IpcMessageType::SetSlideshowActive;
    if (s == "get_slideshow_active")    return IpcMessageType::GetSlideshowActive;
    if (s == "get_seconds_until_next")  return IpcMessageType::GetSecondsUntilNext;
    if (s == "get_health")              return IpcMessageType::GetHealth;
    if (s == "get_display_stats")       return IpcMessageType::GetDisplayStats;
    if (s == "update_check_latest")     return IpcMessageType::UpdateCheckLatest;
    if (s == "update_perform")          return IpcMessageType::UpdatePerform;
    if (s == "update_status")           return IpcMessageType::UpdateStatus;
    if (s == "update_history")          return IpcMessageType::UpdateHistory;
    if (s == "show_status_screen")      return IpcMessageType::ShowStatusScreen;
    if (s == "hide_status_screen")      return IpcMessageType::HideStatusScreen;
    if (s == "show_image")              return IpcMessageType::ShowImage;
    if (s == "get_server_connection")   return IpcMessageType::GetServerConnection;
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
