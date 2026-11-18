#include "config/AppConfig.hpp"
#include "dashboard/ServiceHandlers.hpp"
#include "support/Golden.hpp"
#include "support/Http.hpp"
#include "support/PathShim.hpp"
#include "support/TempDir.hpp"
#include <gtest/gtest.h>

// The services page against s6-svstat lines as the board's s6 prints them.
TEST(ServicesContract, SvstatLinesGiveStatePidAndTimeInState)
{
    PathShim shims;
    shims.add("s6-svstat", "case \"$1\" in"
                           " */shareframe-display) cat '" + golden::hardware("svstat-up.txt").string() + "' ;;"
                           " */shareframe-websocket) cat '" + golden::hardware("svstat-down-signal.txt").string() + "' ;;"
                           " *) cat '" + golden::hardware("svstat-down-exitcode.txt").string() + "' ;; esac");
    TempDir dir;
    AppConfig cfg;
    for (auto* url : {&cfg.ipc.displayRep, &cfg.ipc.wsRep, &cfg.ipc.heartbeatRep, &cfg.ipc.updateRep})
        *url = "ipc://" + (dir.path() / "none.sock").string();
    ServiceHandlers handlers{cfg};

    const auto services = result(handlers.handleList(httpRequest("GET", "/api/services"))).body.at("data").at("services");
    std::map<std::string, nlohmann::json> byId;
    for (const auto& s : services)
        byId[s.at("id")] = s;

    EXPECT_EQ(byId.at("display").at("status"), "up");
    EXPECT_EQ(byId.at("display").at("pid"), 1234);
    EXPECT_EQ(byId.at("display").at("uptime_seconds"), 56);
    EXPECT_EQ(byId.at("websocket").at("status"), "down");
    EXPECT_EQ(byId.at("websocket").at("uptime_seconds"), 56);
    EXPECT_TRUE(byId.at("websocket").at("pid").is_null());
    EXPECT_EQ(byId.at("update").at("status"), "down");
    EXPECT_EQ(byId.at("update").at("uptime_seconds"), 56);
}
