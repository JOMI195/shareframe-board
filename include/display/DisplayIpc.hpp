#pragma once
#include "display/DisplayManager.hpp"
#include "events/EventBus.hpp"
#include "ipc/IpcProtocol.hpp"
#include "repository/ImageRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "task/DisplayImageLoop.hpp"
#include <nlohmann/json.hpp>
#include <string>

/// The display service's IPC surface: REP requests from the dashboard and
/// broadcast events from the websocket service, both translated onto the bus.
class DisplayIpc
{
public:
    DisplayIpc(EventBus& bus, RuntimeSettings& settings, DisplayImageLoop& loop,
               ImageRepository& images, DisplayManager& display);

    /// Commands publish to the bus and ack with {}; queries return data.
    nlohmann::json handle(const IpcMessage& msg);

    void onEvent(const std::string& topic, const nlohmann::json& payload);

private:
    EventBus& bus_;
    RuntimeSettings& settings_;
    DisplayImageLoop& loop_;
    ImageRepository& images_;
    DisplayManager& display_;
};
