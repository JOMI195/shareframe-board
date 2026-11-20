#include "auth/AuthTokenManager.hpp"
#include "dashboard/DashboardServer.hpp"
#include "dashboard/WifiManager.hpp"
#include "repository/ImageRepository.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>
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
    std::string cacheControl;
};

class DashboardServerTest : public ::testing::Test
{
protected:
    DashboardServerTest()
    {
        cfg.dashboardApplication.host = "127.0.0.1";
        cfg.dashboardApplication.port = freePort();
        cfg.secrets.dashboardInitialPassword = "initial-pass";
        cfg.display.setupCodePath = (db.dir() / "setup-code").string();
        cfg.display.safeModePath = (db.dir() / "safe-mode").string();
        cfg.baseUrl = backend.baseUrl();
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
                     const std::string& cookie = "", const std::map<std::string, std::string>& headers = {})
    {
        ix::HttpClient client;
        auto args = client.createRequest();
        args->connectTimeout = 5;
        args->transferTimeout = 20;
        if (!cookie.empty())
            args->extraHeaders["Cookie"] = cookie;
        for (const auto& [name, value] : headers)
            args->extraHeaders[name] = value;
        const auto url = "http://127.0.0.1:" + std::to_string(cfg.dashboardApplication.port) + path;
        const auto r = method == "GET" ? client.get(url, args) : client.post(url, body, args);
        Response out{r->statusCode, nlohmann::json::parse(r->body, nullptr, false), ""};
        if (const auto it = r->headers.find("Set-Cookie"); it != r->headers.end())
            out.setCookie = it->second;
        if (const auto it = r->headers.find("Connection"); it != r->headers.end())
            out.connection = it->second;
        if (const auto it = r->headers.find("Cache-Control"); it != r->headers.end())
            out.cacheControl = it->second;
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
    FakeHttpServer backend;
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
             {"POST", "/api/connection/connect"}, {"POST", "/api/system/safe-mode/exit"},
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
TEST_F(DashboardServerTest, NothingResetsTheFrameWithoutTheCodeFromThePanel)
{
    const auto marker = db.dir() / "reset-ran";
    shims.add("shareframe-factory-reset", "touch " + marker.string());
    shims.add("shareframe-remote-gate", "exit 0");
    const auto cookie = session();
    for (const auto& mode : {"connected", "ap"})
    {
        wifiMode(mode);
        EXPECT_EQ(request("POST", "/api/system/factory-reset", R"({"code":"123456"})").status, 409) << mode;
        EXPECT_EQ(request("POST", "/api/system/factory-reset", R"({"code":"123456"})", cookie).status, 409) << mode;
        EXPECT_EQ(request("POST", "/api/system/factory-reset", "", cookie).status, 400) << mode;
    }
    EXPECT_FALSE(std::filesystem::exists(marker));
}

TEST_F(DashboardServerTest, TheHotspotPasswordIsHiddenFromTheLanButNotFromTheOwner)
{
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["ap_password"], "");
    char host[256] = {};
    gethostname(host, sizeof host - 1);
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["hostname"], host) << "the address the dashboard names";
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

TEST_F(DashboardServerTest, ANightModeUpdateWithoutEnabledIsRefusedInsteadOfTurningItOff)
{
    const auto r = request("POST", "/api/frame/slideshow/night-mode",
                           R"({"start_hour":22,"end_hour":6,"interval_seconds":3600,"style":"hold"})", session());
    EXPECT_EQ(r.status, 400);
}

TEST_F(DashboardServerTest, AWrongTypedFieldIsABadRequestAndTheServerKeepsRunning)
{
    const auto cookie = session();
    const auto r = request("POST", "/api/frame/slideshow/interval", R"({"interval_seconds":"x"})", cookie);
    EXPECT_EQ(r.status, 400);
    EXPECT_EQ(r.body["success"], false);
    EXPECT_EQ(request("POST", "/api/auth/change-password", "[1]", cookie).status, 400);
    EXPECT_EQ(request("GET", "/api/auth/status", "", cookie).status, 200);
}

TEST_F(DashboardServerTest, ACrossOriginPostIsRefusedButAnotherPortOfTheSameHostIsNot)
{
    const auto cookie = session();
    EXPECT_EQ(request("POST", "/api/auth/logout", "", cookie, {{"Origin", "http://evil.example"}}).status, 403);
    EXPECT_EQ(login("initial-pass").status, 200);
    EXPECT_EQ(request("POST", "/api/auth/login", R"({"password":"initial-pass"})", "",
                      {{"Origin", "http://evil.example"}}).status, 403);
    EXPECT_EQ(request("POST", "/api/auth/logout", "", cookie, {{"Origin", "http://127.0.0.1:8090"}}).status, 200);
}

TEST_F(DashboardServerTest, TheSetupCodeSignsInOnTheHotspotUntilTheFrameJoinsAWifi)
{
    std::ofstream(cfg.display.setupCodePath) << "482913\n";
    const auto setupLogin = [&](const std::string& code)
    {
        return request("POST", "/api/auth/login", nlohmann::json{{"setup_code", code}}.dump());
    };
    EXPECT_EQ(setupLogin("482913").status, 409) << "not on the hotspot";

    wifiMode("ap");
    EXPECT_EQ(setupLogin("000000").status, 401);
    const auto r = setupLogin("482913");
    ASSERT_EQ(r.status, 200);
    const auto cookie = r.setCookie.substr(0, r.setCookie.find(';'));

    wifiMode("connecting");
    EXPECT_EQ(request("GET", "/api/auth/status", "", cookie).body["data"]["authenticated"], true)
        << "a connect attempt that fails brings the hotspot back to the same session";
    wifiMode("connected");
    EXPECT_EQ(request("GET", "/api/services", "", cookie).status, 401);
    wifiMode("ap");
    EXPECT_EQ(request("GET", "/api/services", "", cookie).status, 401) << "ended for good";
}

TEST_F(DashboardServerTest, OnlyAPasswordSessionNeedsTheCurrentPasswordAndAChangeEndsTheOthers)
{
    const auto changeTo = [&](const std::string& cookie, const std::string& password)
    {
        return request("POST", "/api/auth/change-password", nlohmann::json{{"new_password", password}}.dump(), cookie);
    };
    const auto other = session();
    const auto passwordSession = session();
    EXPECT_EQ(request("GET", "/api/auth/status", "", passwordSession).body["data"]["password_change_needs_current"], true);
    EXPECT_EQ(changeTo(passwordSession, "new-password-1").status, 401);

    std::ofstream(cfg.display.setupCodePath) << "482913\n";
    wifiMode("ap");
    const auto r = request("POST", "/api/auth/login", R"({"setup_code":"482913"})");
    const auto setupSession = r.setCookie.substr(0, r.setCookie.find(';'));
    EXPECT_EQ(request("GET", "/api/auth/status", "", setupSession).body["data"]["password_change_needs_current"], false);
    EXPECT_EQ(changeTo(setupSession, "new-password-1").status, 200);

    EXPECT_EQ(request("GET", "/api/services", "", other).status, 401);
    EXPECT_EQ(request("GET", "/api/services", "", passwordSession).status, 401);
    EXPECT_EQ(request("GET", "/api/services", "", setupSession).status, 200);
    EXPECT_EQ(login("new-password-1").status, 200);
}

TEST_F(DashboardServerTest, ParallelGuessesDoNotSlipPastTheThrottle)
{
    std::atomic<int> checked = 0;
    std::vector<std::thread> guesses;
    for (int i = 0; i < 20; ++i)
        guesses.emplace_back([&] { checked += login("wrong").status == 401; });
    for (auto& t : guesses)
        t.join();
    EXPECT_LE(checked, LoginThrottle::kFreeFailures);
}

TEST_F(DashboardServerTest, OneClientsLockDoesNotKeepTheOwnerOut)
{
    const auto guess = [&](const std::string& ip, const std::string& password)
    {
        return request("POST", "/api/auth/login", nlohmann::json{{"password", password}}.dump(), "",
                       {{"X-Real-IP", ip}});
    };
    for (int i = 0; i < LoginThrottle::kFreeFailures; ++i)
        EXPECT_EQ(guess("192.168.4.20", "wrong").status, 401);
    EXPECT_EQ(guess("192.168.4.20", "initial-pass").status, 429);
    EXPECT_EQ(guess("192.168.4.21", "initial-pass").status, 200);
}

TEST_F(DashboardServerTest, AOneTimeCodeLoginGivesUpOnAHangingServer)
{
    const auto verify = "POST " + cfg.dashboardApplication.httpVerifyOtpUrl;
    backend.on(verify, 200, R"({"valid":true})");
    backend.during(verify, [] { std::this_thread::sleep_for(std::chrono::seconds(12)); });
    const auto started = std::chrono::steady_clock::now();
    const auto r = request("POST", "/api/auth/login", R"({"otp":"123456"})");
    EXPECT_EQ(r.status, 401);
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(11));
    EXPECT_EQ(backend.requests(cfg.dashboardApplication.httpVerifyOtpUrl).size(), 1u) << "no retries";
}

// With the display down the panel stays blank; the owner sees why and can bring it back.
TEST_F(DashboardServerTest, SafeModeIsShownAndTheOwnerCanLiftIt)
{
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["safe_mode"], "");
    std::ofstream(cfg.display.safeModePath) << "display-crash\n";
    EXPECT_EQ(request("GET", "/api/connection/mode").body["data"]["safe_mode"], "display-crash");

    const auto calls = db.dir() / "safe-mode-calls";
    shims.add("shareframe-safe-mode", "echo \"$@\" >> " + calls.string() + "; echo 'remote: display enabled'");
    const auto r = request("POST", "/api/system/safe-mode/exit", "{}", session());
    EXPECT_EQ(r.status, 200) << r.body.dump();
    std::ifstream f(calls);
    std::string args;
    std::getline(f, args);
    EXPECT_EQ(args, "exit");
}

TEST_F(DashboardServerTest, AFailedSafeModeExitSaysWhy)
{
    shims.add("shareframe-safe-mode", "echo 'display service missing'; exit 1");
    const auto r = request("POST", "/api/system/safe-mode/exit", "{}", session());
    EXPECT_EQ(r.status, 500);
    EXPECT_EQ(r.body["data"]["reason"], "display service missing");
}

// Answers carry sessions, codes and the hotspot password: no browser or proxy keeps them.
TEST_F(DashboardServerTest, JsonAnswersAreNeverCached)
{
    EXPECT_EQ(request("GET", "/api/connection/mode").cacheControl, "no-store");
    EXPECT_EQ(request("GET", "/api/system/info").cacheControl, "no-store") << "the 401";
    EXPECT_EQ(login("wrong").cacheControl, "no-store");
}
