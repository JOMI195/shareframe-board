#pragma once
#include "events/Topic.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "settings/NightWindow.hpp"
#include "settings/PlaybackOrder.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// --- Topic → Message type mapping ---

template <Topic T>
struct TopicMessage; // incomplete primary — compile error if specialization is missing

template <Topic T>
using TopicMessage_t = typename TopicMessage<T>::type;

// Outbound WebSocket message (JSON payload to send to server)
template <> struct TopicMessage<Topic::WS_SEND> { using type = nlohmann::json; };

// WebSocket connection established (fires on every connect and reconnect)
struct WsConnectedEvent {};
template <> struct TopicMessage<Topic::WS_CONNECTED> { using type = WsConnectedEvent; };

// Inbound WebSocket message types (JSON payload from server)
template <> struct TopicMessage<Topic::PICTURE>       { using type = nlohmann::json; };
template <> struct TopicMessage<Topic::CLEAR_IMAGES>  { using type = nlohmann::json; };
template <> struct TopicMessage<Topic::CLEAR_DISPLAY> { using type = nlohmann::json; };
template <> struct TopicMessage<Topic::REMOTE_COMMAND> { using type = nlohmann::json; };

// Internal events
template <> struct TopicMessage<Topic::IMAGE_REMOVED> { using type = std::vector<int64_t>; };
template <> struct TopicMessage<Topic::IMAGE_NEW>     { using type = int64_t; };

// Dashboard IPC events
struct SkipImageEvent {};
struct ShowImageEvent { int64_t id; };
struct UpdateDisplayIntervalEvent { int intervalSecs; };
struct UpdateNightModeEvent { bool enabled; int startHour; int endHour; int intervalSecs; NightStyle style; };
struct UpdatePlaybackOrderEvent { PlaybackMode mode; std::vector<int64_t> customOrder; };
struct SetSlideshowActiveEvent { bool active; };
struct UpdateCustomizationEvent { bool showSender; };

template <> struct TopicMessage<Topic::SKIP_IMAGE>                { using type = SkipImageEvent; };
template <> struct TopicMessage<Topic::SHOW_IMAGE>                { using type = ShowImageEvent; };
template <> struct TopicMessage<Topic::UPDATE_DISPLAY_INTERVAL>   { using type = UpdateDisplayIntervalEvent; };
template <> struct TopicMessage<Topic::UPDATE_NIGHT_MODE>         { using type = UpdateNightModeEvent; };
template <> struct TopicMessage<Topic::UPDATE_PLAYBACK_ORDER>     { using type = UpdatePlaybackOrderEvent; };
template <> struct TopicMessage<Topic::SET_SLIDESHOW_ACTIVE>      { using type = SetSlideshowActiveEvent; };
template <> struct TopicMessage<Topic::UPDATE_CUSTOMIZATION>      { using type = UpdateCustomizationEvent; };

// Wi-Fi setup screen: whether the panel should show it, and the AP to join
struct ApSetupStateEvent
{
    bool required = false;
    std::string ssid;
    std::string password;
    bool operator==(const ApSetupStateEvent&) const = default;
};
template <> struct TopicMessage<Topic::AP_SETUP_STATE> { using type = ApSetupStateEvent; };

// No kind hides the status screen again.
struct StatusScreenEvent
{
    std::optional<StatusScreenKind> kind;
    std::string version;
    bool operator==(const StatusScreenEvent&) const = default;
};
template <> struct TopicMessage<Topic::STATUS_SCREEN> { using type = StatusScreenEvent; };

// The daily maintenance clear wiped the panel; whoever owns it paints its screen again.
struct ScheduledClearEvent {};
template <> struct TopicMessage<Topic::SCHEDULED_CLEAR> { using type = ScheduledClearEvent; };

// Whether the server takes the frame or refuses it because nobody owns it yet.
struct FrameRegistrationEvent
{
    bool registered = false;
};
template <> struct TopicMessage<Topic::FRAME_REGISTRATION> { using type = FrameRegistrationEvent; };
