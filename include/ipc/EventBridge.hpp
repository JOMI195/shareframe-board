#pragma once
#include "ipc/EventPublisher.hpp"

class EventBus;

/// Forwards selected internal EventBus topics out onto the nng PUB socket so the
/// display service can react. Kept separate from WebsocketClient: the WS task
/// stays a pure producer into the internal bus (see feedback_ws_architecture);
/// this bridge is the sole owner of bus → cross-process fan-out.
class EventBridge
{
public:
    EventBridge(EventBus& bus, EventPublisher& pub);

    /// Subscribe to the relevant topics. Must be called before EventBus::start().
    void wire();

private:
    EventBus& bus_;
    EventPublisher& pub_;
};
