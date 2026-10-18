#include "display/DisplayIpc.hpp"
#include "events/Messages.hpp"
#include "ipc/EventTopics.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

DisplayIpc::DisplayIpc(EventBus& bus, RuntimeSettings& settings, DisplayImageLoop& loop,
                       ImageRepository& images, DisplayManager& display)
    : bus_(bus), settings_(settings), loop_(loop), images_(images), display_(display)
{
}

nlohmann::json DisplayIpc::handle(const IpcMessage& msg)
{
    switch (msg.type)
    {
    case IpcMessageType::SkipImage:
        if (loop_.isRefreshing())
        {
            spdlog::info("IPC: skip image rejected, panel refreshing");
            return {{"refreshing", true}};
        }
        spdlog::info("IPC: skip image");
        bus_.publish<Topic::SKIP_IMAGE>(SkipImageEvent{});
        return {{"refreshing", false}};

    case IpcMessageType::ShowImage:
    {
        const int64_t id = msg.data.is_object() ? msg.data.value("id", int64_t{0}) : 0;
        if (loop_.isRefreshing())
        {
            spdlog::info("IPC: show image {} rejected, panel refreshing", id);
            return {{"refreshing", true}};
        }
        if (!settings_.isSlideshowActive())
        {
            spdlog::info("IPC: show image {} rejected, slideshow paused", id);
            return {{"inactive", true}};
        }
        if (images_.getByIds({id}).empty())
        {
            spdlog::info("IPC: show image {} rejected, not stored", id);
            return {{"found", false}};
        }
        spdlog::info("IPC: show image {}", id);
        bus_.publish<Topic::SHOW_IMAGE>(ShowImageEvent{id});
        return {{"refreshing", false}, {"found", true}};
    }

    case IpcMessageType::UpdateDisplayInterval:
    {
        const int secs = msg.data.value("interval_secs", 0);
        if (secs <= 0)
        {
            spdlog::warn("IPC: invalid interval_secs in update_display_interval");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update display interval to {}s", secs);
        bus_.publish<Topic::UPDATE_DISPLAY_INTERVAL>(UpdateDisplayIntervalEvent{secs});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetDisplayInterval:
        return {{"interval_secs", settings_.getDisplayInterval()}};

    case IpcMessageType::UpdateNightMode:
    {
        const bool enabled = msg.data.value("enabled", false);
        const int startHour = msg.data.value("start_hour", -1);
        const int endHour = msg.data.value("end_hour", -1);
        const int secs = msg.data.value("interval_secs", 0);
        const auto style = nightStyleFromString(msg.data.value("style", ""));
        if (startHour < 0 || startHour > 23 || endHour < 0 || endHour > 23
            || startHour == endHour || secs <= 0 || !style)
        {
            spdlog::warn("IPC: invalid payload in update_night_mode");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update night mode to enabled={}, window={}-{}, interval={}s, style={}",
                     enabled, startHour, endHour, secs, toString(*style));
        bus_.publish<Topic::UPDATE_NIGHT_MODE>(
            UpdateNightModeEvent{enabled, startHour, endHour, secs, *style});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetNightMode:
        return {
            {"enabled", settings_.isNightModeEnabled()},
            {"start_hour", settings_.getNightStartHour()},
            {"end_hour", settings_.getNightEndHour()},
            {"interval_secs", settings_.getNightInterval()},
            {"style", toString(settings_.getNightStyle())},
            {"active_now", settings_.isNightNow()}
        };

    case IpcMessageType::UpdatePlaybackOrder:
    {
        const auto mode = playbackModeFromString(msg.data.value("mode", ""));
        std::vector<int64_t> customOrder;
        try { customOrder = msg.data.value("custom_order", std::vector<int64_t>{}); }
        catch (...) { customOrder.clear(); }
        if (!mode)
        {
            spdlog::warn("IPC: invalid mode in update_playback_order");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update playback order to {}", toString(*mode));
        bus_.publish<Topic::UPDATE_PLAYBACK_ORDER>(UpdatePlaybackOrderEvent{*mode, std::move(customOrder)});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetPlaybackOrder:
        return {
            {"mode", toString(settings_.getPlaybackMode())},
            {"custom_order", settings_.getCustomOrder()},
            {"shuffle_order", loop_.shuffleOrder()},
            {"current_image_id", loop_.currentImageId()}
        };

    case IpcMessageType::UpdateCustomization:
    {
        const auto it = msg.data.find("show_sender");
        if (it == msg.data.end() || !it->is_boolean())
        {
            spdlog::warn("IPC: invalid payload in update_customization");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update customization to show_sender={}", it->get<bool>());
        bus_.publish<Topic::UPDATE_CUSTOMIZATION>(UpdateCustomizationEvent{it->get<bool>()});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetCustomization:
        return {{"show_sender", settings_.isShowSender()}};

    case IpcMessageType::GetScreenInfo:
    {
        const auto screen = display_.screenSnapshot();
        int64_t imageId = 0;
        std::string sender;
        if (screen.state == "image")
        {
            const auto all = images_.getAll();
            if (const auto it = std::ranges::find(all, std::filesystem::path(screen.source), &Image::imagePath);
                it != all.end())
            {
                imageId = it->id;
                sender = it->sender;
            }
        }
        return {
            {"seq", screen.seq},
            {"state", screen.state},
            {"source", screen.source},
            {"updated_at", screen.updatedAt},
            {"image_id", imageId},
            {"sender", sender},
        };
    }

    case IpcMessageType::ClearDisplay:
        spdlog::info("IPC: clear display");
        bus_.publish<Topic::CLEAR_DISPLAY>(nlohmann::json{});
        return nlohmann::json::object();

    case IpcMessageType::SetSlideshowActive:
    {
        const bool active = msg.data.value("active", true);
        spdlog::info("IPC: set slideshow active -> {}", active);
        bus_.publish<Topic::SET_SLIDESHOW_ACTIVE>(SetSlideshowActiveEvent{active});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetSlideshowActive:
        return {
            {"active", settings_.isSlideshowActive()},
            {"loop_started", loop_.isReady()},
            {"refreshing", loop_.isRefreshing()},
            {"image_count", static_cast<int64_t>(images_.getAllIds().size())}
        };

    case IpcMessageType::GetSecondsUntilNext:
        return {{"seconds_until_next", loop_.secondsUntilNext()}};

    case IpcMessageType::GetHealth:
        return {{"running", true}};

    case IpcMessageType::GetDisplayStats:
        return display_.healthSnapshot();

    case IpcMessageType::ShowStatusScreen:
    {
        const auto kind = statusScreenKindFromString(msg.data.value("screen", ""));
        if (!kind)
        {
            spdlog::warn("IPC: invalid screen in show_status_screen");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: show status screen {}", toString(*kind));
        bus_.publish<Topic::STATUS_SCREEN>(StatusScreenEvent{kind, msg.data.value("version", "")});
        return nlohmann::json::object();
    }

    case IpcMessageType::HideStatusScreen:
        spdlog::info("IPC: hide status screen");
        bus_.publish<Topic::STATUS_SCREEN>(StatusScreenEvent{});
        return nlohmann::json::object();

    default:
        return nlohmann::json::object();
    }
}

void DisplayIpc::onEvent(const std::string& topic, const nlohmann::json& payload)
{
    if (topic == event_topics::DisplayClear)
    {
        bus_.publish<Topic::CLEAR_DISPLAY>(payload);
    }
    else if (topic == event_topics::ImageRemoved)
    {
        bus_.publish<Topic::IMAGE_REMOVED>(payload.value("ids", std::vector<int64_t>{}));
    }
    else if (topic == event_topics::ImageNew)
    {
        // Wake the loop so defaults yield to a freshly arrived image.
        spdlog::debug("Event: new image {}", payload.value("id", int64_t{0}));
        bus_.publish<Topic::IMAGE_NEW>(payload.value("id", int64_t{0}));
    }
}
