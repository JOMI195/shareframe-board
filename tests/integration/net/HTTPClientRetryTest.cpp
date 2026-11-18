#include "net/HTTPClient.hpp"
#include "support/FakeHttpServer.hpp"
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <ixwebsocket/IXHttpServer.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

// Requests to the Shareframe server ride out a server that is briefly unreachable, but never
// repeat a request the server did answer.
namespace
{
int freePort()
{
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    socklen_t len = sizeof addr;
    getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
    close(fd);
    return ntohs(addr.sin_port);
}
}

TEST(HTTPClientRetry, AServerThatComesUpWhileRetryingIsReached)
{
    const int port = freePort();
    ix::HttpServer late(port, "127.0.0.1");
    late.setOnConnectionCallback([](const ix::HttpRequestPtr&, const std::shared_ptr<ix::ConnectionState>&)
    {
        return std::make_shared<ix::HttpResponse>(200, "OK", ix::HttpErrorCode::Ok, ix::WebSocketHttpHeaders{}, "up");
    });
    std::thread starter([&] { std::this_thread::sleep_for(1500ms); late.listen(); late.start(); });

    HTTPClient http{1, 2};
    const auto r = http.get("http://127.0.0.1:" + std::to_string(port) + "/");
    starter.join();
    late.stop();
    EXPECT_EQ(r.statusCode, 200);
    EXPECT_EQ(r.body, "up");
}

TEST(HTTPClientRetry, AnAnswerIsNeverRetriedEvenAnError)
{
    FakeHttpServer backend;
    backend.on("POST /api/x", 503);
    backend.on("GET /api/y", 401);
    HTTPClient http{1, 2};
    EXPECT_EQ(http.post("http://" + backend.baseUrl() + "/api/x", "{}").statusCode, 503);
    EXPECT_EQ(http.get("http://" + backend.baseUrl() + "/api/y").statusCode, 401);
    EXPECT_EQ(backend.requests().size(), 2u);
}
