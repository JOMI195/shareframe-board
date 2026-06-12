#pragma once
#include "ipc/EventPublisher.hpp"

class EventBus;

/// Forwards selected EventBus topics onto the nng PUB socket for the display
/// service. Sole owner of bus → cross-process fan-out (the WS task stays a pure
/// producer into the bus).
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
