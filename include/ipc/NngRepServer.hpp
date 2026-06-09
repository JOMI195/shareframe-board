#pragma once
#include "ipc/IpcProtocol.hpp"
#include <functional>
#include <nng/nng.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stop_token>
#include <string>
#include <thread>

/// Request/response server over nng REP. Generic: the owning service supplies a
/// handler that turns a parsed IpcMessage into the response data. Because
/// REQ/REP is lockstep, the handler is called for *every* request and its return
/// value is always sent back (an empty object acts as a command ack).
class NngRepServer
{
public:
    using Handler = std::function<nlohmann::json(const IpcMessage&)>;

    NngRepServer(std::string url, Handler handler);
    ~NngRepServer();

    void start();
    void stop();

private:
    void _serveLoop(const std::stop_token& st);

    std::string url_;
    Handler handler_;
    std::shared_ptr<spdlog::logger> logger_;
    nng_socket sock_{};
    bool open_ = false;
    std::jthread thread_;
};
