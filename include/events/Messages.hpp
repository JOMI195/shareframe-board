#pragma once
#include "events/Topic.hpp"
#include <nlohmann/json.hpp>

// --- Topic → Message type mapping ---

template <Topic T>
struct TopicMessage; // incomplete primary — compile error if specialization is missing

template <Topic T>
using TopicMessage_t = typename TopicMessage<T>::type;

// Outbound WebSocket message (JSON payload to send to server)
template <> struct TopicMessage<Topic::WS_SEND> { using type = nlohmann::json; };

// Extend as server message types are defined:
//
// namespace msg {
//     struct FrameUpdate { int frame_id; std::string url; };
// }
// template <> struct TopicMessage<Topic::FRAME_UPDATE> { using type = msg::FrameUpdate; };
