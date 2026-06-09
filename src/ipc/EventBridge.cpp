#include "ipc/EventBridge.hpp"
#include "events/EventBus.hpp"
#include "ipc/EventTopics.hpp"

EventBridge::EventBridge(EventBus& bus, EventPublisher& pub)
    : bus_(bus), pub_(pub)
{
}

void EventBridge::wire()
{
    bus_.subscribe<Topic::IMAGE_NEW>([this](const int64_t& id)
    {
        pub_.publish(std::string(event_topics::ImageNew), {{"id", id}});
    });

    bus_.subscribe<Topic::IMAGE_REMOVED>([this](const std::vector<int64_t>& ids)
    {
        pub_.publish(std::string(event_topics::ImageRemoved), {{"ids", ids}});
    });

    bus_.subscribe<Topic::CLEAR_DISPLAY>([this](const nlohmann::json& msg)
    {
        pub_.publish(std::string(event_topics::DisplayClear), msg);
    });
}
