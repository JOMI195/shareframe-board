#pragma once
#include <mutex>
#include <nlohmann/json.hpp>
#include <nng/nng.h>
#include <spdlog/spdlog.h>
#include <string>

/// Fire-and-forget broadcast over nng PUB. Messages are framed as
/// "<topic>\n<json>" so subscribers can prefix-filter by topic. Binds the
/// endpoint (subscribers dial in).
class EventPublisher
{
public:
    explicit EventPublisher(std::string url);
    ~EventPublisher();

    bool start();
    void publish(const std::string& topic, const nlohmann::json& payload);

private:
    std::string url_;
    std::shared_ptr<spdlog::logger> logger_;
    nng_socket sock_{};
    bool open_ = false;
    std::mutex mtx_;
};
