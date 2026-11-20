#include "ipc/NngRepServer.hpp"
#include "ipc/IpcSocketPath.hpp"
#include <nng/protocol/reqrep0/rep.h>
#include <algorithm>
#include <utility>


NngRepServer::NngRepServer(std::string url, Handler handler, const int workers)
    : url_(std::move(url)), handler_(std::move(handler)),
      logger_(spdlog::default_logger()->clone("NngRepServer")), workers_(std::max(1, workers))
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

    ipc_socket::prepare(url_);
    if (const int rv = nng_listen(sock_, url_.c_str(), nullptr, 0); rv != 0)
    {
        logger_->error("nng_listen({}) failed: {}", url_, nng_strerror(rv));
        nng_close(sock_);
        return;
    }

    open_ = true;
    logger_->info("REP server listening on {}", url_);
    // Each worker answers on its own context, so a slow request holds up only its own.
    for (int i = 0; i < workers_; ++i)
    {
        nng_ctx ctx{};
        if (const int rv = nng_ctx_open(&ctx, sock_); rv != 0)
        {
            logger_->error("nng_ctx_open failed: {}", nng_strerror(rv));
            continue;
        }
        // Bounded recv so the worker can observe stop requests.
        nng_ctx_set_ms(ctx, NNG_OPT_RECVTIMEO, 500);
        threads_.emplace_back([this, ctx](const std::stop_token& st) { _serveLoop(st, ctx); });
    }
}

void NngRepServer::stop()
{
    for (auto& t : threads_)
        t.request_stop();
    threads_.clear();
    if (open_)
    {
        nng_close(sock_);
        open_ = false;
        ipc_socket::remove(url_);
        logger_->info("REP server stopped ({})", url_);
    }
}

void NngRepServer::_serveLoop(const std::stop_token& st, const nng_ctx ctx)
{
    while (!st.stop_requested())
    {
        nng_msg* request = nullptr;
        const int rv = nng_ctx_recvmsg(ctx, &request, 0);
        if (rv == NNG_ETIMEDOUT)
            continue;
        if (rv != 0)
        {
            if (!st.stop_requested())
                logger_->error("nng_recv failed: {}", nng_strerror(rv));
            break;
        }

        nlohmann::json responseData = nlohmann::json::object();
        const std::string body(static_cast<const char*>(nng_msg_body(request)), nng_msg_len(request));
        if (const auto msg = parseIpcMessage(body))
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

        // REQ/REP requires a reply for every request, even unknown ones; the request's message carries it back.
        const std::string reply = toJsonResponse(responseData);
        nng_msg_clear(request);
        if (const int arv = nng_msg_append(request, reply.data(), reply.size()); arv != 0)
        {
            logger_->error("nng reply too large: {}", nng_strerror(arv));
            nng_msg_free(request);
            continue;
        }
        if (const int srv = nng_ctx_sendmsg(ctx, request, 0); srv != 0)
        {
            logger_->error("nng_send reply failed: {}", nng_strerror(srv));
            nng_msg_free(request);
        }
    }
    nng_ctx_close(ctx);
}
