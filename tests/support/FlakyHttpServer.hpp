#pragma once
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <string_view>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

/// Serves one body over raw HTTP so a test can drop the connection mid-body,
/// which ix::HttpServer cannot do. Honors `Range: bytes=N-` with a 206.
class FlakyHttpServer
{
public:
    explicit FlakyHttpServer(std::string body) : body_(std::move(body))
    {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 || ::listen(fd_, 8) != 0)
            throw std::runtime_error("FlakyHttpServer: cannot listen");
        socklen_t len = sizeof addr;
        ::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
        thread_ = std::thread([this] { _serve(); });
    }

    ~FlakyHttpServer()
    {
        stop_ = true;
        thread_.join();
        ::close(fd_);
    }

    FlakyHttpServer(const FlakyHttpServer&) = delete;
    FlakyHttpServer& operator=(const FlakyHttpServer&) = delete;

    /// The next `times` responses send only `bytes` of their body, then hang up.
    void cutAfter(size_t bytes, int times)
    {
        std::lock_guard lk(mtx_);
        cutBytes_ = bytes;
        cutsLeft_ = times;
    }

    /// Answer every request with the whole body and a 200.
    void ignoreRange()
    {
        std::lock_guard lk(mtx_);
        ignoreRange_ = true;
    }

    /// Answer requests that carry a Range with this status instead of a 206.
    void refuseResumeWith(int status)
    {
        std::lock_guard lk(mtx_);
        resumeStatus_ = status;
    }

    /// The Range value of each request in order, empty when it had none.
    [[nodiscard]] std::vector<std::string> ranges() const
    {
        std::lock_guard lk(mtx_);
        return ranges_;
    }

    [[nodiscard]] std::string url() const { return "http://127.0.0.1:" + std::to_string(port_) + "/bundle"; }

private:
    void _serve()
    {
        while (!stop_)
        {
            pollfd p{fd_, POLLIN, 0};
            if (::poll(&p, 1, 50) <= 0)
                continue;
            const int c = ::accept(fd_, nullptr, nullptr);
            if (c < 0)
                continue;
            _answer(c);
            ::close(c);
        }
    }

    void _answer(int c)
    {
        std::string req;
        char buf[1024];
        while (req.find("\r\n\r\n") == std::string::npos)
        {
            const auto n = ::recv(c, buf, sizeof buf, 0);
            if (n <= 0)
                return;
            req.append(buf, static_cast<size_t>(n));
        }
        std::string range;
        constexpr std::string_view kRange = "Range: bytes=";
        if (const auto pos = req.find(kRange); pos != std::string::npos)
        {
            const auto from = pos + kRange.size();
            range = req.substr(from, req.find("\r\n", from) - from);
        }

        std::lock_guard lk(mtx_);
        ranges_.push_back(range);
        int status = 200;
        size_t offset = 0;
        if (!range.empty() && !ignoreRange_)
        {
            status = resumeStatus_ ? resumeStatus_ : 206;
            offset = status == 206 ? std::stoul(range) : 0;
        }
        const std::string payload = status >= 400 ? "{}" : body_.substr(offset);
        size_t sendBytes = payload.size();
        if (cutsLeft_ > 0)
        {
            --cutsLeft_;
            sendBytes = std::min(sendBytes, cutBytes_);
        }
        const std::string out = "HTTP/1.1 " + std::to_string(status) + " X\r\nContent-Length: "
            + std::to_string(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload.substr(0, sendBytes);
        for (size_t sent = 0; sent < out.size();)
        {
            const auto n = ::send(c, out.data() + sent, out.size() - sent, MSG_NOSIGNAL);
            if (n <= 0)
                return;
            sent += static_cast<size_t>(n);
        }
    }

    std::string body_;
    int fd_ = -1;
    int port_ = 0;
    std::atomic<bool> stop_{false};
    std::thread thread_;
    mutable std::mutex mtx_;
    size_t cutBytes_ = 0;
    int cutsLeft_ = 0;
    bool ignoreRange_ = false;
    int resumeStatus_ = 0;
    std::vector<std::string> ranges_;
};
