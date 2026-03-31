#include "ipc/IpcClient.hpp"
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

IpcClient::IpcClient(const std::string& socketPath)
    : socketPath_(socketPath),
      logger_(spdlog::default_logger()->clone("IpcClient"))
{
}

IpcClient::~IpcClient()
{
    disconnect();
}

bool IpcClient::connect()
{
    std::lock_guard lk(mtx_);
    if (fd_ >= 0)
        return true;

    fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_ < 0)
    {
        logger_->error("Failed to create socket: {}", strerror(errno));
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        logger_->error("Failed to connect to {}: {}", socketPath_, strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    logger_->info("Connected to IPC socket {}", socketPath_);
    return true;
}

void IpcClient::disconnect()
{
    std::lock_guard lk(mtx_);
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
        logger_->info("Disconnected from IPC socket");
    }
}

bool IpcClient::isConnected() const
{
    return fd_ >= 0;
}

bool IpcClient::send(const IpcMessage& msg)
{
    std::lock_guard lk(mtx_);
    if (fd_ < 0)
    {
        logger_->error("Cannot send IPC message: not connected");
        return false;
    }

    std::string data = toJson(msg); // includes trailing newline
    ssize_t sent = ::send(fd_, data.c_str(), data.size(), MSG_NOSIGNAL);
    if (sent < 0)
    {
        logger_->error("Failed to send IPC message: {}", strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

std::optional<nlohmann::json> IpcClient::sendAndReceive(
    const IpcMessage& msg, std::chrono::milliseconds timeout)
{
    std::lock_guard lk(mtx_);
    if (fd_ < 0)
    {
        logger_->error("Cannot send IPC message: not connected");
        return std::nullopt;
    }

    // Send
    std::string data = toJson(msg);
    ssize_t sent = ::send(fd_, data.c_str(), data.size(), MSG_NOSIGNAL);
    if (sent < 0)
    {
        logger_->error("Failed to send IPC message: {}", strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return std::nullopt;
    }

    // Wait for response with timeout
    pollfd pfd{fd_, POLLIN, 0};
    int ret = ::poll(&pfd, 1, static_cast<int>(timeout.count()));
    if (ret <= 0)
    {
        logger_->error("IPC response timeout or poll error");
        return std::nullopt;
    }

    // Read response (one newline-delimited JSON line)
    std::string buffer;
    char chunk[1024];
    while (true)
    {
        ssize_t n = ::recv(fd_, chunk, sizeof(chunk), 0);
        if (n <= 0)
        {
            logger_->error("Failed to read IPC response");
            ::close(fd_);
            fd_ = -1;
            return std::nullopt;
        }
        buffer.append(chunk, static_cast<size_t>(n));

        if (auto pos = buffer.find('\n'); pos != std::string::npos)
        {
            std::string line = buffer.substr(0, pos);
            try
            {
                auto j = nlohmann::json::parse(line);
                return j.value("data", nlohmann::json{});
            }
            catch (...)
            {
                logger_->error("Failed to parse IPC response: {}", line);
                return std::nullopt;
            }
        }
    }
}
