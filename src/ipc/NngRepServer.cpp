#include "ipc/NngRepServer.hpp"
#include <nng/protocol/reqrep0/rep.h>
#include <unistd.h>
#include <utility>

namespace
{
// nng's ipc:// transport binds a Unix socket file; remove a stale one first so a
// crashed predecessor doesn't block listen().
void unlinkIfIpc(const std::string& url)
{
    constexpr std::string_view prefix = "ipc://";
    if (url.starts_with(prefix))
        ::unlink(url.substr(prefix.size()).c_str());
}
} // namespace

NngRepServer::NngRepServer(std::string url, Handler handler)
    : url_(std::move(url)), handler_(std::move(handler)),
      logger_(spdlog::default_logger()->clone("NngRepServer"))
{
}

NngRepServer::~NngRepServer()
{
    stop();
}

void NngRepServer::start()
{
    if (const int rv = nng_rep0_open(&sock_); rv != 0)
    {
        logger_->error("nng_rep0_open failed: {}", nng_strerror(rv));
        return;
    }

    // Bounded recv so the worker can observe stop requests.
    nng_socket_set_ms(sock_, NNG_OPT_RECVTIMEO, 500);

    unlinkIfIpc(url_);
    if (const int rv = nng_listen(sock_, url_.c_str(), nullptr, 0); rv != 0)
    {
        logger_->error("nng_listen({}) failed: {}", url_, nng_strerror(rv));
        nng_close(sock_);
        return;
    }

    open_ = true;
    logger_->info("REP server listening on {}", url_);
    thread_ = std::jthread([this](const std::stop_token& st) { _serveLoop(st); });
}

void NngRepServer::stop()
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
        unlinkIfIpc(url_);
        logger_->info("REP server stopped ({})", url_);
    }
}

void NngRepServer::_serveLoop(const std::stop_token& st)
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

        nlohmann::json responseData = nlohmann::json::object();
        if (const auto msg = parseIpcMessage(std::string(buf, len)))
        {
            try
            {
                responseData = handler_(*msg);
            }
            catch (const std::exception& e)
            {
                logger_->error("REP handler threw: {}", e.what());
            }
        }
        else
        {
            logger_->warn("Invalid IPC request on {}", url_);
        }
        nng_free(buf, len);

        // REQ/REP requires a reply for every request, even unknown ones.
        const std::string reply = toJsonResponse(responseData);
        if (const int srv = nng_send(sock_, const_cast<char*>(reply.data()), reply.size(), 0); srv != 0)
            logger_->error("nng_send reply failed: {}", nng_strerror(srv));
    }
}
