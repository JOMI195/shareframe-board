#include "config/AppConfig.hpp"
#include "dashboard/ServiceHandlers.hpp"
#include "support/Http.hpp"
#include "support/PathShim.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class ServiceHandlersTest : public ::testing::Test
{
protected:
    ServiceHandlersTest()
    {
        shims.add("s6-svc", "echo \"$*\" >> " + calls.string());
        for (auto* url : {&cfg.ipc.displayRep, &cfg.ipc.wsRep, &cfg.ipc.heartbeatRep, &cfg.ipc.updateRep})
            *url = "ipc://" + (dir.path() / "none.sock").string();
    }

    HttpResult restart(const std::string& body)
    {
        return result(handlers.handleRestart(httpRequest("POST", "/api/services/restart", nlohmann::json::parse(body))));
    }

    std::string svcCalls() const
    {
        std::ifstream f(calls);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    PathShim shims;
    TempDir dir;
    std::filesystem::path calls = dir.path() / "s6-svc.log";
    AppConfig cfg;
    ServiceHandlers handlers{cfg};
};
}

TEST_F(ServiceHandlersTest, RestartGoesToTheServicesSupervisor)
{
    EXPECT_EQ(restart(R"({"service":"heartbeat"})").status, 200);
    EXPECT_EQ(svcCalls(), "-r /run/service/shareframe-heartbeat\n");
}

// The id ends up in a path: anything but the five app services is refused before s6 sees it.
TEST_F(ServiceHandlersTest, OnlyTheAppServicesCanBeRestarted)
{
    for (const auto* body : {R"({"service":"dropbear"})", R"({"service":"../../usr/bin/x"})", R"({"service":""})", "{}"})
        EXPECT_EQ(restart(body).status, 400) << body;
    EXPECT_EQ(svcCalls(), "");
}

TEST_F(ServiceHandlersTest, AFailingSupervisorIsReported)
{
    shims.add("s6-svc", "echo 's6-svc: fatal: unable to control' >&2; exit 111");
    const auto r = restart(R"({"service":"display"})");
    EXPECT_EQ(r.status, 500);
    EXPECT_EQ(r.body["success"], false);
}

TEST_F(ServiceHandlersTest, WithoutS6EveryServiceIsUnknownInsteadOfAnError)
{
    shims.add("s6-svstat", "exit 111");
    const auto r = result(handlers.handleList(httpRequest("GET", "/api/services")));
    EXPECT_EQ(r.status, 200);
    for (const auto& s : r.body["data"]["services"])
        EXPECT_EQ(s["status"], "unknown") << s["id"];
}
