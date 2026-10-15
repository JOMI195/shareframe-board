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
        spdlog::info("IPC: skip image");
        bus_.publish<Topic::SKIP_IMAGE>(SkipImageEvent{});
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
        const bool blank = msg.data.value("blank", false);
        if (startHour < 0 || startHour > 23 || endHour < 0 || endHour > 23
            || startHour == endHour || secs <= 0)
        {
            spdlog::warn("IPC: invalid payload in update_night_mode");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update night mode to enabled={}, window={}-{}, interval={}s, blank={}",
                     enabled, startHour, endHour, secs, blank);
        bus_.publish<Topic::UPDATE_NIGHT_MODE>(
            UpdateNightModeEvent{enabled, startHour, endHour, secs, blank});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetNightMode:
        return {
            {"enabled", settings_.isNightModeEnabled()},
            {"start_hour", settings_.getNightStartHour()},
            {"end_hour", settings_.getNightEndHour()},
            {"interval_secs", settings_.getNightInterval()},
            {"blank", settings_.isNightBlank()},
            {"active_now", settings_.isNightNow()}
        };

    case IpcMessageType::UpdateImageQuality:
    {
        const auto quality = imageQualityFromString(msg.data.value("quality", ""));
        if (!quality)
        {
            spdlog::warn("IPC: invalid quality in update_image_quality");
            return nlohmann::json::object();
        }
        spdlog::info("IPC: update image quality to {}", toString(*quality));
        bus_.publish<Topic::UPDATE_IMAGE_QUALITY>(UpdateImageQualityEvent{*quality});
        return nlohmann::json::object();
    }

    case IpcMessageType::GetImageQuality:
        return {{"quality", toString(settings_.getImageQuality())}};

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
            {"image_count", static_cast<int64_t>(images_.getAllIds().size())}
        };

    case IpcMessageType::GetSecondsUntilNext:
        return {{"seconds_until_next", loop_.secondsUntilNext()}};

    case IpcMessageType::GetHealth:
        return {{"running", true}};

    case IpcMessageType::GetDisplayStats:
        return display_.healthSnapshot();

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
