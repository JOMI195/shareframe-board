#pragma once
#include <string_view>

/// Wire-level event topic names broadcast WS → Display over nng PUB/SUB.
/// Shared so the publisher (EventBridge) and subscriber (display service) agree.
namespace event_topics
{
inline constexpr std::string_view ImageNew     = "image.new";
inline constexpr std::string_view ImageRemoved = "image.removed";
inline constexpr std::string_view DisplayClear = "display.clear";
} // namespace event_topics
