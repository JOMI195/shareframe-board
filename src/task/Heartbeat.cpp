#include "task/Heartbeat.hpp"
#include "events/Topic.hpp"
#include <nlohmann/json.hpp>

Heartbeat::Heartbeat(EventBus& bus, const AppConfig& cfg)
    : bus_(bus), cfg_(cfg), logger_(spdlog::default_logger()->clone("Heartbeat"))
{
}

void Heartbeat::start()
{
    logger_->info("Starting heartbeat thread (interval={}s)", cfg_.heartbeat.intervalSecs);
    thread_ = std::jthread([this](std::stop_token st) { _run(std::move(st)); });
}

void Heartbeat::stop()
{
    logger_->info("Stopping heartbeat thread");
    thread_.request_stop();
    cv_.notify_all();
    if (thread_.joinable())
        thread_.join();
    logger_->info("Heartbeat thread stopped");
}

void Heartbeat::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        _sendHeartbeat();

        std::unique_lock lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(cfg_.heartbeat.intervalSecs), [&st]
        {
            return st.stop_requested();
        });
    }
}

void Heartbeat::_sendHeartbeat()
{
    const nlohmann::json payload = {{"type", "heartbeat"}};
    logger_->debug("Publishing heartbeat to bus: {}", payload.dump());
    bus_.publish<Topic::WS_SEND>(payload);
}
