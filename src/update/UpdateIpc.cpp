#include "update/UpdateIpc.hpp"
#include <spdlog/spdlog.h>

UpdateIpc::UpdateIpc(UpdateManager& updates) : updates_(updates)
{
}

nlohmann::json UpdateIpc::handle(const IpcMessage& msg)
{
    switch (msg.type)
    {
    case IpcMessageType::UpdateCheckLatest:
    {
        auto resp = updates_.fetchLatestRelease();
        if (!resp.ok())
            return {{"ok", false}, {"error", resp.errorMsg}, {"status_code", resp.statusCode}};
        try
        {
            auto release = nlohmann::json::parse(resp.body);
            if (release.is_object())
            {
                const auto version = release.value("version", "");
                release["is_newer"] = updates_.isNewerThanInstalled(version);
            }
            return {{"ok", true}, {"release", release}};
        }
        catch (...)
        {
            return {{"ok", false}, {"error", "Invalid response from update server"}};
        }
    }

    case IpcMessageType::UpdatePerform:
    {
        std::string error;
        const bool started = updates_.startUpdate(error);
        return {{"ok", started}, {"error", error}};
    }

    case IpcMessageType::UpdateStatus:
        return {{"ok", true}, {"status", updates_.status()}};

    case IpcMessageType::UpdateHistory:
        return {{"ok", true}, {"history", updates_.history()}};

    case IpcMessageType::GetHealth:
        return {{"running", true}};

    default:
        spdlog::warn("IPC: unhandled message type");
        return nlohmann::json::object();
    }
}
