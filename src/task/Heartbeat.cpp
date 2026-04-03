#include "task/Heartbeat.hpp"
#include "events/Topic.hpp"
#include "net/WsProtocol.hpp"
#include <nlohmann/json.hpp>

Heartbeat::Heartbeat(EventBus& bus, const AppConfig& cfg)
    : PeriodicTask(bus, cfg, "Heartbeat")
{
}

int Heartbeat::intervalSecs() const
{
    return cfg_.heartbeat.intervalSecs;
}

void Heartbeat::execute()
{
    const nlohmann::json payload = {{"type", wsMessageTypeToString(WsMessageType::Heartbeat)}};
    logger_->debug("Publishing heartbeat to bus: {}", payload.dump());
    bus_.publish<Topic::WS_SEND>(payload);
}
