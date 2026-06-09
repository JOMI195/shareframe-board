#include "ipc/EventPublisher.hpp"
#include <nng/protocol/pubsub0/pub.h>
#include <unistd.h>
#include <utility>

EventPublisher::EventPublisher(std::string url)
    : url_(std::move(url)),
      logger_(spdlog::default_logger()->clone("EventPublisher"))
{
}

EventPublisher::~EventPublisher()
{
    std::lock_guard lk(mtx_);
    if (open_)
    {
        nng_close(sock_);
        open_ = false;
        if (url_.starts_with("ipc://"))
            ::unlink(url_.substr(6).c_str());
    }
}

bool EventPublisher::start()
{
    std::lock_guard lk(mtx_);
    if (const int rv = nng_pub0_open(&sock_); rv != 0)
    {
        logger_->error("nng_pub0_open failed: {}", nng_strerror(rv));
        return false;
    }
    if (url_.starts_with("ipc://"))
        ::unlink(url_.substr(6).c_str());
    if (const int rv = nng_listen(sock_, url_.c_str(), nullptr, 0); rv != 0)
    {
        logger_->error("nng_listen({}) failed: {}", url_, nng_strerror(rv));
        nng_close(sock_);
        return false;
    }
    open_ = true;
    logger_->info("PUB publisher listening on {}", url_);
    return true;
}

void EventPublisher::publish(const std::string& topic, const nlohmann::json& payload)
{
    std::lock_guard lk(mtx_);
    if (!open_)
        return;
    const std::string wire = topic + "\n" + payload.dump();
    if (const int rv = nng_send(sock_, const_cast<char*>(wire.data()), wire.size(), 0); rv != 0)
        logger_->error("nng_send (topic {}) failed: {}", topic, nng_strerror(rv));
}
