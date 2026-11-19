#include "auth/AuthTokenManager.hpp"
#include "dashboard/DashboardServer.hpp"
#include "dashboard/WifiManager.hpp"
#include "repository/ImageRepository.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <gtest/gtest.h>
#include <ixwebsocket/IXHttpClient.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// The dashboard as the browser sees it: the real server on a port, its auth gate and sessions.
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

struct Response
{
    int status;
    nlohmann::json body;
    std::string setCookie;
    std::string connection;
};

class DashboardServerTest : public ::testing::Test
{
protected:
    DashboardServerTest()
    {
        cfg.dashboardApplication.host = "127.0.0.1";
        cfg.dashboardApplication.port = freePort();
        cfg.secrets.dashboardInitialPassword = "initial-pass";
        wifiMode("connected");
        shims.add("shareframe-factory-reset", "exit 0");
        shims.add("reboot", "exit 0");
        start();
    }

    ~DashboardServerTest() override { server->stop(); }

    void start()
    {
        server = std::make_unique<DashboardServer>(cfg, ipc, http, sessions, auth, wifi, updateIpc, wsIpc, settings, images);
        server->start();
    }

    void wifiMode(const std::string& mode)
    {
        std::ofstream(wifiModePath) << nlohmann::json{{"mode", mode}, {"ssid", "home"}, {"ap_ssid", "frame"},
                                                      {"ap_password", "hotspot-pass"}}.dump();
    }

    Response request(const std::string& method, const std::string& path, const std::string& body = "",
                     const std::string& cookie = "")
    {
        ix::HttpClient client;
        auto args = client.createRequest();
        args->connectTimeout = 5;
        args->transferTimeout = 10;
        if (!cookie.empty())
            args->extraHeaders["Cookie"] = cookie;
        const auto url = "http://127.0.0.1:" + std::to_string(cfg.dashboardApplication.port) + path;
        const auto r = method == "GET" ? client.get(url, args) : client.post(url, body, args);
        Response out{r->statusCode, nlohmann::json::parse(r->body, nullptr, false), ""};
        if (const auto it = r->headers.find("Set-Cookie"); it != r->headers.end())
            out.setCookie = it->second;
        if (const auto it = r->headers.find("Connection"); it != r->headers.end())
            out.connection = it->second;
        return out;
    }

    Response login(const std::string& password)
    {
        return request("POST", "/api/auth/login", nlohmann::json{{"password", password}}.dump());
    }

    /// The cookie a successful login sets, ready for the Cookie header.
    std::string session(const std::string& password = "initial-pass")
    {
        const auto r = login(password);
        EXPECT_EQ(r.status, 200);
        return r.setCookie.substr(0, r.setCookie.find(';'));
    }

    PathShim shims;
    TestDb db;
    AppConfig cfg;
    std::string wifiModePath = (db.dir() / "wifi-mode.json").string();
    std::string sock = "ipc://" + (db.dir() / "none.sock").string();
    IpcClient ipc{sock}, updateIpc{sock}, wsIpc{sock};
    HTTPClient http{1, 1};
    SessionManager sessions;
    TokenRepository tokens{db.get()};
    AuthTokenManager auth{cfg, tokens, http};
    WifiManager wifi{wifiModePath};
    SettingsRepository settings{db.get()};
    ImageRepository images{db.get()};
    std::unique_ptr<DashboardServer> server;
};
}

TEST_F(DashboardServerTest, EveryProtectedRouteNeedsASession)
{
    for (const auto& [method, path] : std::vector<std::pair<std::string, std::string>>{
             {"GET", "/api/system/info"}, {"GET", "/api/services"}, {"POST", "/api/frame/slideshow/skip"},
             {"POST", "/api/connection/connect"}, {"POST", "/api/system/factory-reset"},
             {"POST", "/api/auth/change-password"}, {"GET", "/api/system/updates/status"}})
    {
        EXPECT_EQ(request(method, path).status, 401) << method << " " << path;
        EXPECT_EQ(request(method, path, "", "session=forged").status, 401) << method << " " << path;
    }
}

TEST_F(DashboardServerTest, LoginSetsAnHttpOnlySessionCookieThatOpensTheDashboard)
{
    const auto r = login("initial-pass");
    ASSERT_EQ(r.status, 200);
    EXPECT_NE(r.setCookie.find("HttpOnly"), std::string::npos);
    EXPECT_NE(r.setCookie.find("SameSite=Lax"), std::string::npos);
    EXPECT_NE(r.setCookie.find("Path=/"), std::string::npos);
    EXPECT_EQ(r.setCookie.find("Max-Age"), std::string::npos) << "the board decides when a session ends";

    const auto cookie = r.setCookie.substr(0, r.setCookie.find(';'));
    EXPECT_EQ(request("GET", "/api/auth/status", "", cookie).body["data"]["authenticated"], true);
    EXPECT_EQ(request("GET", "/api/services", "", cookie).status, 200);
}

// Other cookies on the board's origin must not hide or fake the session.
TEST_F(DashboardServerTest, TheSessionIsFoundAmongOtherCookies)
{
    const auto cookie = session();
    EXPECT_EQ(request("GET", "/api/services", "", "a_session=nope; " + cookie + "; theme=dark").status, 200);
    EXPECT_EQ(request("GET", "/api/services", "", "theme=" + cookie.substr(8)).status, 401);
}

TEST_F(DashboardServerTest, AWrongPasswordFailsAndFiveOfThemLockEvenTheRightOne)
{
    EXPECT_EQ(login("wrong-pass").status, 401);
    for (int i = 0; i < 4; ++i)
        login("wrong-pass");
    EXPECT_EQ(login("initial-pass").status, 429);
}

TEST_F(DashboardServerTest, AChangedPasswordReplacesTheInitialOneAndSurvivesARestart)
{
    const auto cookie = session();
    const auto body = nlohmann::json{{"current_password", "initial-pass"}, {"new_password", "owner-pass-1"}}.dump();
    ASSERT_EQ(request("POST", "/api/auth/change-password", body, cookie).status, 200);
    EXPECT_EQ(login("initial-pass").status, 401);

    server->stop();
    start();
    EXPECT_EQ(login("owner-pass-1").status, 200);
}

TEST_F(DashboardServerTest, ChangingThePasswordNeedsTheCurrentOne)
{
    const auto cookie = session();
    const auto body = nlohmann::json{{"current_password", "guess-pass"}, {"new_password", "owner-pass-1"}}.dump();
    EXPECT_EQ(request("POST", "/api/auth/change-password", body, cookie).status, 401);
    EXPECT_EQ(login("initial-pass").status, 200);
}

TEST_F(DashboardServerTest, LogoutEndsTheSessionOnTheBoard)
{
    const auto cookie = session();
    ASSERT_EQ(request("POST", "/api/auth/logout", "", cookie).status, 200);
    EXPECT_EQ(request("GET", "/api/services", "", cookie).status, 401);
}

// On the hotspot a forgotten password must not lock the owner out for good.
TEST_F(DashboardServerTest, FactoryResetNeedsNoLoginOnlyOnTheHotspot)
{
    EXPECT_EQ(request("POST", "/api/system/factory-reset").status, 401);
    wifiMode("ap");
    EXPECT_NE(request("POST", "/api/system/factory-reset").status, 401);
}

TEST_F(DashboardServerTest, TheHotspotPasswordIsHiddenFromTheLanButNotFromTheOwner)
{
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["ap_password"], "");
    EXPECT_EQ(request("GET", "/api/connection/mode", "", session()).body["data"]["ap_password"], "hotspot-pass");
    wifiMode("ap");
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["ap_password"], "hotspot-pass");
}

// The server closes every connection after one answer; a client that reused it would lose its next request.
TEST_F(DashboardServerTest, EveryAnswerSaysTheConnectionCloses)
{
    EXPECT_EQ(login("initial-pass").connection, "close");
    EXPECT_EQ(login("wrong").connection, "close");
    EXPECT_EQ(request("GET", "/api/system/info").connection, "close");
    EXPECT_EQ(request("GET", "/api/nope", "", session()).connection, "close");
}

TEST_F(DashboardServerTest, UnknownRoutesAndBrokenBodiesGetTheEnvelope)
{
    const auto notFound = request("GET", "/api/nope", "", session());
    EXPECT_EQ(notFound.status, 404);
    EXPECT_EQ(notFound.body["success"], false);
    const auto bad = request("POST", "/api/auth/login", "{");
    EXPECT_EQ(bad.status, 400);
    EXPECT_EQ(bad.body["success"], false);
}
