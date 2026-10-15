#pragma once
#include "ipc/IpcProtocol.hpp"
#include "update/UpdateManager.hpp"
#include <nlohmann/json.hpp>

/// The update service's REP surface; the dashboard proxies /api/system/updates/* here.
class UpdateIpc
{
public:
    explicit UpdateIpc(UpdateManager& updates);

    nlohmann::json handle(const IpcMessage& msg);

private:
    UpdateManager& updates_;
};
