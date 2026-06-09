#pragma once
#include <functional>
#include <nlohmann/json.hpp>
#include <nng/nng.h>
#include <spdlog/spdlog.h>
#include <stop_token>
#include <string>
#include <thread>

/// Receives broadcast events from an EventPublisher over nng SUB. Subscribes to
/// all topics and hands each ("<topic>", parsed-json) to the callback on a
/// worker thread. Dials the publisher's endpoint (reconnects automatically).
class EventSubscriber
{
public:
    using Callback = std::function<void(const std::string& topic, const nlohmann::json& payload)>;

    EventSubscriber(std::string url, Callback cb);
    ~EventSubscriber();

    void start();
    void stop();

private:
    void _recvLoop(const std::stop_token& st);

    std::string url_;
    Callback cb_;
    std::shared_ptr<spdlog::logger> logger_;
    nng_socket sock_{};
    bool open_ = false;
    std::jthread thread_;
};
