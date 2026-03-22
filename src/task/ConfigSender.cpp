#include "task/ConfigSender.hpp"
#include "events/Topic.hpp"
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <unistd.h>

ConfigSender::ConfigSender(EventBus& bus, const AppConfig& cfg)
    : bus_(bus), cfg_(cfg), logger_(spdlog::default_logger()->clone("ConfigSender"))
{
}

void ConfigSender::start()
{
    logger_->info("Starting config sender thread (interval={}s)", cfg_.configSender.intervalSecs);
    thread_ = std::jthread([this](std::stop_token st) { _run(std::move(st)); });
}

void ConfigSender::stop()
{
    logger_->info("Stopping config sender thread");
    thread_.request_stop();
    cv_.notify_all();
    if (thread_.joinable())
        thread_.join();
    logger_->info("Config sender thread stopped");
}

void ConfigSender::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        _sendConfig();

        std::unique_lock lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(cfg_.configSender.intervalSecs), [&st]
        {
            return st.stop_requested();
        });
    }
}

void ConfigSender::_sendConfig() const
{
    auto localIp = _getLocalIp();
    if (localIp.empty())
    {
        logger_->warn("Could not determine local IP address for config message");
        return;
    }

    const nlohmann::json payload = {
        {"type", "config"},
        {"local_ip_address", localIp},
        {"version", cfg_.version}
    };

    logger_->debug("Publishing config to bus: {}", payload.dump());
    bus_.publish<Topic::WS_SEND>(payload);
}

std::string ConfigSender::_getLocalIp()
{
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return {};

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        close(fd);
        return {};
    }

    sockaddr_in localAddr{};
    socklen_t len = sizeof(localAddr);
    getsockname(fd, reinterpret_cast<sockaddr*>(&localAddr), &len);
    close(fd);

    char buf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &localAddr.sin_addr, buf, sizeof(buf));
    return buf;
}
