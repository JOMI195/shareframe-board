#pragma once
#include "display/DisplayManager.hpp"
#include "display/ScreenSelection.hpp"
#include "events/EventBus.hpp"
#include "ipc/IpcProtocol.hpp"
#include "repository/ImageRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "task/DisplayImageLoop.hpp"
#include <functional>
#include <nlohmann/json.hpp>
#include <string>

/// The display service's IPC surface: REP requests from the dashboard and
/// broadcast events from the websocket service, both translated onto the bus.
class DisplayIpc
{
public:
    /// With a screen source the display stats also name what the panel shows.
    DisplayIpc(EventBus& bus, RuntimeSettings& settings, DisplayImageLoop& loop,
               ImageRepository& images, DisplayManager& display, std::function<Screen()> screen = {});

    /// Commands publish to the bus and ack with {}; queries return data.
    nlohmann::json handle(const IpcMessage& msg);

    void onEvent(const std::string& topic, const nlohmann::json& payload);

private:
    EventBus& bus_;
    RuntimeSettings& settings_;
    DisplayImageLoop& loop_;
    ImageRepository& images_;
    DisplayManager& display_;
    std::function<Screen()> screen_;
};
