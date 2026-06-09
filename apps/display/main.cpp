#include "app/Bootstrap.hpp"
#include "db/Database.hpp"
#include "display/DisplayManager.hpp"
#include "events/EventBus.hpp"
#include "events/Messages.hpp"
#include "ipc/EventSubscriber.hpp"
#include "ipc/EventTopics.hpp"
#include "ipc/IpcProtocol.hpp"
#include "ipc/NngRepServer.hpp"
#include "repository/ImageRepository.hpp"
#include "repository/SettingsRepository.hpp"
#include "settings/RuntimeSettings.hpp"
#include "task/DisplayClear.hpp"
#include "task/DisplayImageLoop.hpp"
#include <spdlog/spdlog.h>

// Owns all display behavior. Commands/queries arrive over nng REP (from the
// dashboard + heartbeat); broadcast events arrive over nng SUB (from the
// websocket service). Both feed the internal EventBus, which the display tasks
// consume.
int main(int argc, char* argv[])
{
    auto [cfg, profile] = bootstrap(argc, argv);
    initLogging(cfg, cfg.displayApplication.logFile);
    spdlog::info("shareframe-display v{} starting [profile: {}]", cfg.version, profileName(profile));

    // Reads/writes its own state only; migrations are run by the standalone
    // shareframe-migrate oneshot before any service starts, so open without
    // migrating.
    Database database;
    database.init(cfg.database, false);
    ImageRepository imageRepo(database.get());
    SettingsRepository settingsRepo(database.get());
    RuntimeSettings runtimeSettings(settingsRepo, cfg);

    EventBus eventBus;

    DisplayManager displayManager(cfg);
    displayManager.init();

    // Display tasks subscribe to the bus — wire them before eventBus.start().
    DisplayImageLoop displayImageLoop(eventBus, cfg, imageRepo, displayManager, runtimeSettings);
    displayImageLoop.start();
    DisplayClear displayClear(eventBus, cfg, displayManager);
    displayClear.start();

    eventBus.start();

    // REP handler: reproduces the former IpcServer dispatch. Commands publish to
    // the bus and ack with {}; queries return data. Every request gets a reply
    // (REQ/REP is lockstep).
    NngRepServer repServer(cfg.ipc.displayRep, [&](const IpcMessage& msg) -> nlohmann::json
    {
        switch (msg.type)
        {
        case IpcMessageType::SkipImage:
            spdlog::info("IPC: skip image");
            eventBus.publish<Topic::SKIP_IMAGE>(SkipImageEvent{});
            return nlohmann::json::object();

        case IpcMessageType::UpdateDisplayInterval:
        {
            const int secs = msg.data.value("interval_secs", 0);
            if (secs <= 0)
            {
                spdlog::warn("IPC: invalid interval_secs in update_display_interval");
                return nlohmann::json::object();
            }
            spdlog::info("IPC: update display interval to {}s", secs);
            eventBus.publish<Topic::UPDATE_DISPLAY_INTERVAL>(UpdateDisplayIntervalEvent{secs});
            return nlohmann::json::object();
        }

        case IpcMessageType::GetDisplayInterval:
            return {{"interval_secs", runtimeSettings.getDisplayInterval()}};

        case IpcMessageType::ClearDisplay:
            spdlog::info("IPC: clear display");
            eventBus.publish<Topic::CLEAR_DISPLAY>(nlohmann::json{});
            return nlohmann::json::object();

        case IpcMessageType::SetSlideshowActive:
        {
            const bool active = msg.data.value("active", true);
            spdlog::info("IPC: set slideshow active -> {}", active);
            eventBus.publish<Topic::SET_SLIDESHOW_ACTIVE>(SetSlideshowActiveEvent{active});
            return nlohmann::json::object();
        }

        case IpcMessageType::GetSlideshowActive:
            return {{"active", runtimeSettings.isSlideshowActive()}};

        case IpcMessageType::GetHealth:
            return {{"running", true}};
        }
        return nlohmann::json::object();
    });
    repServer.start();

    // SUB: broadcast events from the websocket service → internal bus.
    EventSubscriber subscriber(cfg.ipc.wsPub, [&](const std::string& topic, const nlohmann::json& payload)
    {
        if (topic == event_topics::DisplayClear)
        {
            eventBus.publish<Topic::CLEAR_DISPLAY>(payload);
        }
        else if (topic == event_topics::ImageRemoved)
        {
            eventBus.publish<Topic::IMAGE_REMOVED>(payload.value("ids", std::vector<int64_t>{}));
        }
        else if (topic == event_topics::ImageNew)
        {
            // The display loop re-scans the image set every cycle, so a new image
            // is picked up without an explicit nudge — log for observability only.
            spdlog::debug("Event: new image {}", payload.value("id", int64_t{0}));
        }
    });
    subscriber.start();

    const int sig = waitForSignal();
    spdlog::info("Received signal {}, shutting down", sig);
    subscriber.stop();
    repServer.stop();
    eventBus.stop();
    displayClear.stop();
    displayImageLoop.stop();

    return 0;
}
