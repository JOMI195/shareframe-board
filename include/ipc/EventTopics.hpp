#pragma once
#include <string_view>

/// Wire-level event topic names broadcast from the WS service over nng PUB/SUB.
/// Shared so the publisher (EventBridge) and subscribers (display, heartbeat) agree.
namespace event_topics
{
inline constexpr std::string_view ImageNew     = "image.new";
inline constexpr std::string_view ImageRemoved = "image.removed";
inline constexpr std::string_view DisplayClear = "display.clear";
inline constexpr std::string_view WsConnected  = "ws.connected";
} // namespace event_topics
