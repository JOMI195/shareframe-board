#pragma once
#include "ipc/IpcProtocol.hpp"
#include "net/ServerContact.hpp"
#include <nlohmann/json.hpp>

/// The websocket service's REP surface: health probes and the server link state.
class WsIpc
{
public:
    explicit WsIpc(const ServerContact& contact);

    nlohmann::json handle(const IpcMessage& msg) const;

private:
    const ServerContact& contact_;
};
