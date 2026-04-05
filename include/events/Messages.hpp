#pragma once
#include "events/Topic.hpp"
#include <nlohmann/json.hpp>
#include <vector>

// --- Topic → Message type mapping ---

template <Topic T>
struct TopicMessage; // incomplete primary — compile error if specialization is missing

template <Topic T>
using TopicMessage_t = typename TopicMessage<T>::type;

// Outbound WebSocket message (JSON payload to send to server)
template <> struct TopicMessage<Topic::WS_SEND> { using type = nlohmann::json; };

// Inbound WebSocket message types (JSON payload from server)
template <> struct TopicMessage<Topic::PICTURE>       { using type = nlohmann::json; };
template <> struct TopicMessage<Topic::CLEAR_IMAGES>  { using type = nlohmann::json; };
template <> struct TopicMessage<Topic::CLEAR_DISPLAY> { using type = nlohmann::json; };

// Internal events
template <> struct TopicMessage<Topic::IMAGE_REMOVED> { using type = std::vector<int64_t>; };

// Dashboard IPC events
struct SkipImageEvent {};
struct UpdateDisplayIntervalEvent { int intervalSecs; };
struct SetSlideshowActiveEvent { bool active; };

template <> struct TopicMessage<Topic::SKIP_IMAGE>                { using type = SkipImageEvent; };
template <> struct TopicMessage<Topic::UPDATE_DISPLAY_INTERVAL>   { using type = UpdateDisplayIntervalEvent; };
template <> struct TopicMessage<Topic::SET_SLIDESHOW_ACTIVE>      { using type = SetSlideshowActiveEvent; };
