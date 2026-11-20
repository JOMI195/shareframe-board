#pragma once
#include "ipc/IpcProtocol.hpp"
#include <functional>
#include <nng/nng.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

/// Request/response server over nng REP. The owning service supplies a handler
/// turning an IpcMessage into response data; its return is always sent back
/// (empty object = command ack). With more than one worker, requests are handled
/// in parallel, so the handler must be thread-safe.
class NngRepServer
{
public:
    using Handler = std::function<nlohmann::json(const IpcMessage&)>;

    NngRepServer(std::string url, Handler handler, int workers = 1);
    ~NngRepServer();

    void start();
    void stop();

private:
    void _serveLoop(const std::stop_token& st, nng_ctx ctx);

    std::string url_;
    Handler handler_;
    std::shared_ptr<spdlog::logger> logger_;
    nng_socket sock_{};
    bool open_ = false;
    int workers_;
    std::vector<std::jthread> threads_;
};
