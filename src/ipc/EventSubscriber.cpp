#include "ipc/EventSubscriber.hpp"
#include <nng/protocol/pubsub0/sub.h>
#include <utility>

EventSubscriber::EventSubscriber(std::string url, Callback cb)
    : url_(std::move(url)), cb_(std::move(cb)),
      logger_(spdlog::default_logger()->clone("EventSubscriber"))
{
}

EventSubscriber::~EventSubscriber()
{
    stop();
}

void EventSubscriber::start()
{
    if (const int rv = nng_sub0_open(&sock_); rv != 0)
    {
        logger_->error("nng_sub0_open failed: {}", nng_strerror(rv));
        return;
    }

    // Empty prefix = subscribe to everything; filtering happens in the callback.
    if (const int rv = nng_socket_set(sock_, NNG_OPT_SUB_SUBSCRIBE, "", 0); rv != 0)
    {
        logger_->error("nng SUB subscribe-all failed: {}", nng_strerror(rv));
        nng_close(sock_);
        return;
    }

    nng_socket_set_ms(sock_, NNG_OPT_RECVTIMEO, 500);

    if (const int rv = nng_dial(sock_, url_.c_str(), nullptr, NNG_FLAG_NONBLOCK); rv != 0)
    {
        logger_->error("nng_dial({}) failed: {}", url_, nng_strerror(rv));
        nng_close(sock_);
        return;
    }

    open_ = true;
    logger_->info("SUB subscriber dialing {}", url_);
    thread_ = std::jthread([this](const std::stop_token& st) { _recvLoop(st); });
}

void EventSubscriber::stop()
{
    if (thread_.joinable())
    {
        thread_.request_stop();
        thread_.join();
    }
    if (open_)
    {
        nng_close(sock_);
        open_ = false;
        logger_->info("SUB subscriber stopped ({})", url_);
    }
}

void EventSubscriber::_recvLoop(const std::stop_token& st)
{
    while (!st.stop_requested())
    {
        char* buf = nullptr;
        size_t len = 0;
        const int rv = nng_recv(sock_, &buf, &len, NNG_FLAG_ALLOC);
        if (rv == NNG_ETIMEDOUT)
            continue;
        if (rv != 0)
        {
            if (!st.stop_requested())
                logger_->error("nng_recv failed: {}", nng_strerror(rv));
            break;
        }

        const std::string wire(buf, len);
        nng_free(buf, len);

        const auto nl = wire.find('\n');
        if (nl == std::string::npos)
        {
            logger_->warn("Malformed event (no topic delimiter)");
            continue;
        }
        const std::string topic = wire.substr(0, nl);
        try
        {
            const auto payload = nlohmann::json::parse(wire.substr(nl + 1));
            cb_(topic, payload);
        }
        catch (const std::exception& e)
        {
            logger_->error("Failed to handle event '{}': {}", topic, e.what());
        }
    }
}
