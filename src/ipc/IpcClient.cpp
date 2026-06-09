#include "ipc/IpcClient.hpp"
#include <nng/protocol/reqrep0/req.h>
#include <utility>

IpcClient::IpcClient(std::string url)
    : url_(std::move(url)),
      logger_(spdlog::default_logger()->clone("IpcClient"))
{
}

IpcClient::~IpcClient()
{
    disconnect();
}

bool IpcClient::_ensure()
{
    if (open_)
        return true;

    if (const int rv = nng_req0_open(&sock_); rv != 0)
    {
        logger_->error("nng_req0_open failed: {}", nng_strerror(rv));
        return false;
    }

    // Keep reconnect backoff tight so a restarted peer is picked up quickly.
    nng_socket_set_ms(sock_, NNG_OPT_RECONNMINT, 100);
    nng_socket_set_ms(sock_, NNG_OPT_RECONNMAXT, 2000);

    // NONBLOCK: the dialer keeps retrying in the background, so the peer
    // service is allowed to start after us.
    if (const int rv = nng_dial(sock_, url_.c_str(), nullptr, NNG_FLAG_NONBLOCK); rv != 0)
    {
        logger_->error("nng_dial({}) failed: {}", url_, nng_strerror(rv));
        nng_close(sock_);
        return false;
    }

    open_ = true;
    logger_->info("Dialing IPC endpoint {}", url_);
    return true;
}

bool IpcClient::connect()
{
    std::lock_guard lk(mtx_);
    return _ensure();
}

void IpcClient::disconnect()
{
    std::lock_guard lk(mtx_);
    if (open_)
    {
        nng_close(sock_);
        open_ = false;
        logger_->info("Closed IPC endpoint {}", url_);
    }
}

bool IpcClient::isConnected() const
{
    return open_;
}

bool IpcClient::send(const IpcMessage& msg)
{
    // REQ/REP is lockstep — complete the round-trip but ignore the ack body.
    return sendAndReceive(msg).has_value();
}

std::optional<nlohmann::json> IpcClient::sendAndReceive(
    const IpcMessage& msg, std::chrono::milliseconds timeout)
{
    std::lock_guard lk(mtx_);
    if (!_ensure())
        return std::nullopt;

    nng_socket_set_ms(sock_, NNG_OPT_SENDTIMEO, static_cast<nng_duration>(timeout.count()));
    nng_socket_set_ms(sock_, NNG_OPT_RECVTIMEO, static_cast<nng_duration>(timeout.count()));

    const std::string payload = toJson(msg); // trailing newline harmless over nng framing
    if (const int rv = nng_send(sock_, const_cast<char*>(payload.data()), payload.size(), 0); rv != 0)
    {
        logger_->error("nng_send to {} failed: {}", url_, nng_strerror(rv));
        return std::nullopt;
    }

    char* buf = nullptr;
    size_t len = 0;
    if (const int rv = nng_recv(sock_, &buf, &len, NNG_FLAG_ALLOC); rv != 0)
    {
        logger_->error("nng_recv from {} failed: {}", url_, nng_strerror(rv));
        return std::nullopt;
    }

    std::optional<nlohmann::json> result;
    try
    {
        auto j = nlohmann::json::parse(std::string(buf, len));
        result = j.value("data", nlohmann::json{});
    }
    catch (...)
    {
        logger_->error("Failed to parse IPC response from {}", url_);
    }
    nng_free(buf, len);
    return result;
}
