#include "ipc/IpcServer.hpp"
#include "events/Messages.hpp"
#include "ipc/IpcProtocol.hpp"
#include "settings/RuntimeSettings.hpp"
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

IpcServer::IpcServer(EventBus& bus, const AppConfig& cfg, RuntimeSettings& settings)
    : bus_(bus), cfg_(cfg), settings_(settings),
      logger_(spdlog::default_logger()->clone("IpcServer"))
{
}

IpcServer::~IpcServer()
{
    stop();
}

void IpcServer::start()
{
    const auto& path = cfg_.dashboardApplication.socketPath;

    // Remove stale socket file
    ::unlink(path.c_str());

    serverFd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd_ < 0)
    {
        logger_->error("Failed to create Unix socket: {}", strerror(errno));
        return;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        logger_->error("Failed to bind socket to {}: {}", path, strerror(errno));
        ::close(serverFd_);
        serverFd_ = -1;
        return;
    }

    if (::listen(serverFd_, 4) < 0)
    {
        logger_->error("Failed to listen on socket: {}", strerror(errno));
        ::close(serverFd_);
        serverFd_ = -1;
        return;
    }

    logger_->info("IPC server listening on {}", path);
    acceptThread_ = std::jthread([this](const std::stop_token& st) { _acceptLoop(st); });
}

void IpcServer::stop()
{
    if (acceptThread_.joinable())
        acceptThread_.request_stop();

    if (serverFd_ >= 0)
    {
        ::close(serverFd_);
        serverFd_ = -1;
    }

    if (acceptThread_.joinable())
        acceptThread_.join();

    {
        std::lock_guard lk(clientMtx_);
        for (auto& t : clientThreads_)
        {
            if (t.joinable())
            {
                t.request_stop();
                t.join();
            }
        }
        clientThreads_.clear();
    }

    ::unlink(cfg_.dashboardApplication.socketPath.c_str());
    logger_->info("IPC server stopped");
}

void IpcServer::_acceptLoop(const std::stop_token& st)
{
    while (!st.stop_requested())
    {
        pollfd pfd{serverFd_, POLLIN, 0};
        const int ret = ::poll(&pfd, 1, 500); // 500ms timeout for stop checks
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            logger_->error("poll() failed: {}", strerror(errno));
            break;
        }
        if (ret == 0 || !(pfd.revents & POLLIN))
            continue;

        int clientFd = ::accept(serverFd_, nullptr, nullptr);
        if (clientFd < 0)
        {
            if (errno == EINTR || errno == EBADF)
                continue;
            logger_->error("accept() failed: {}", strerror(errno));
            continue;
        }

        logger_->info("IPC client connected (fd={})", clientFd);

        std::lock_guard lk(clientMtx_);
        clientThreads_.emplace_back(
            [this, clientFd](const std::stop_token& cst) { _handleClient(clientFd, cst); });
    }
}

void IpcServer::_handleClient(int clientFd, const std::stop_token& st) const
{
    std::string buffer;
    char chunk[1024];

    while (!st.stop_requested())
    {
        pollfd pfd{clientFd, POLLIN, 0};
        const int ret = ::poll(&pfd, 1, 500);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ret == 0)
            continue;

        const ssize_t n = ::recv(clientFd, chunk, sizeof(chunk), 0);
        if (n <= 0)
        {
            if (n == 0)
                logger_->info("IPC client disconnected (fd={})", clientFd);
            else
                logger_->error("recv() failed on fd {}: {}", clientFd, strerror(errno));
            break;
        }

        buffer.append(chunk, static_cast<size_t>(n));

        // Process complete newline-delimited messages
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!line.empty())
                _dispatch(clientFd, line);
        }
    }

    ::close(clientFd);
}

void IpcServer::_dispatch(const int clientFd, const std::string& line) const
{
    const auto msg = parseIpcMessage(line);
    if (!msg)
    {
        logger_->warn("Invalid IPC message: {}", line);
        return;
    }

    switch (msg->type)
    {
    case IpcMessageType::SkipImage:
        logger_->info("IPC: skip image");
        bus_.publish<Topic::SKIP_IMAGE>(SkipImageEvent{});
        break;

    case IpcMessageType::UpdateDisplayInterval:
        {
            int secs = msg->data.value("interval_secs", 0);
            if (secs <= 0)
            {
                logger_->warn("IPC: invalid interval_secs in update_display_interval");
                return;
            }
            logger_->info("IPC: update display interval to {}s", secs);
            bus_.publish<Topic::UPDATE_DISPLAY_INTERVAL>(UpdateDisplayIntervalEvent{secs});
            break;
        }

    case IpcMessageType::GetDisplayInterval:
        {
            int secs = settings_.getDisplayInterval();
            logger_->debug("IPC: get_display_interval -> {}s", secs);
            _sendResponse(clientFd, {{"interval_secs", secs}});
            break;
        }

    case IpcMessageType::ClearDisplay:
        logger_->info("IPC: clear display");
        bus_.publish<Topic::CLEAR_DISPLAY>(nlohmann::json{});
        break;

    case IpcMessageType::SetSlideshowActive:
        {
            bool active = msg->data.value("active", true);
            logger_->info("IPC: set slideshow active -> {}", active);
            bus_.publish<Topic::SET_SLIDESHOW_ACTIVE>(SetSlideshowActiveEvent{active});
            break;
        }

    case IpcMessageType::GetSlideshowActive:
        {
            bool active = settings_.isSlideshowActive();
            logger_->debug("IPC: get_slideshow_active -> {}", active);
            _sendResponse(clientFd, {{"active", active}});
            break;
        }

    case IpcMessageType::GetHealth:
        // Health hook: the process answering at all means it is running. Always
        // true for now; can later fold in internal state (display/WS health).
        logger_->debug("IPC: get_health -> running");
        _sendResponse(clientFd, {{"running", true}});
        break;
    }
}

void IpcServer::_sendResponse(int clientFd, const nlohmann::json& data) const
{
    const std::string resp = toJsonResponse(data);
    size_t totalSent = 0;
    while (totalSent < resp.size())
    {
        ssize_t n = ::send(clientFd, resp.c_str() + totalSent,
                           resp.size() - totalSent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            logger_->error("IPC send failed on fd {}: {}", clientFd, strerror(errno));
            return;
        }
        totalSent += static_cast<size_t>(n);
    }
}
