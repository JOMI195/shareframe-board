#include "net/WsIpc.hpp"
#include "ipc/HealthCheck.hpp"

WsIpc::WsIpc(const ServerContact& contact) : contact_(contact)
{
}

nlohmann::json WsIpc::handle(const IpcMessage& msg) const
{
    if (msg.type == IpcMessageType::GetServerConnection)
        return toJson(contact_.snapshot());
    return health::respond(msg);
}
