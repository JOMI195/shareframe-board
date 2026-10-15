#include "display/DisplayIpc.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

// What the display service accepts when a request bypasses the dashboard's validation.
namespace
{
class DisplayIpcTest : public ::testing::Test, protected ImageFixture
{
protected:
    DisplayIpcTest()
    {
        cfg.display.mockDisplay = true;
        bus.start();
    }

    ~DisplayIpcTest() override { bus.stop(); }

    SettingsRepository settingsRepo{db.get()};
    RuntimeSettings settings{settingsRepo, cfg};
    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayImageLoop loop{bus, cfg, repo, display, settings};
    DisplayIpc ipc{bus, settings, loop, repo, display};
    Recorder<Topic::UPDATE_DISPLAY_INTERVAL> intervals{bus};
    Recorder<Topic::UPDATE_NIGHT_MODE> nightModes{bus};
};
}

TEST_F(DisplayIpcTest, MalformedSettingsAreAckedButIgnored)
{
    for (const nlohmann::json& data : {nlohmann::json::object(), {{"interval_secs", 0}}, {{"interval_secs", -5}}})
        EXPECT_EQ(ipc.handle({IpcMessageType::UpdateDisplayInterval, data}), nlohmann::json::object()) << data;

    for (const nlohmann::json& data : {
             nlohmann::json{{"start_hour", 24}, {"end_hour", 5}, {"interval_secs", 60}},
             nlohmann::json{{"start_hour", 2}, {"end_hour", -1}, {"interval_secs", 60}},
             nlohmann::json{{"start_hour", 2}, {"end_hour", 2}, {"interval_secs", 60}},
             nlohmann::json{{"start_hour", 2}, {"end_hour", 5}, {"interval_secs", 0}},
             nlohmann::json::object()})
        EXPECT_EQ(ipc.handle({IpcMessageType::UpdateNightMode, data}), nlohmann::json::object()) << data;

    ipc.handle({IpcMessageType::UpdateDisplayInterval, {{"interval_secs", 600}}});
    ASSERT_TRUE(waitFor([&] { return intervals.size() == 1; }));
    EXPECT_EQ(intervals.get()[0].intervalSecs, 600);
    EXPECT_EQ(nightModes.size(), 0u);
}

TEST_F(DisplayIpcTest, AnswersHealthAndAcksForeignMessages)
{
    EXPECT_EQ(ipc.handle({IpcMessageType::GetHealth, {}}), (nlohmann::json{{"running", true}}));
    EXPECT_EQ(ipc.handle({IpcMessageType::UpdatePerform, {}}), nlohmann::json::object());
}

TEST_F(DisplayIpcTest, NightModeQueryReportsWhetherTheWindowIsActive)
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    settings.setNightMode(true, local.tm_hour, (local.tm_hour + 1) % 24, 3600);

    const auto night = ipc.handle({IpcMessageType::GetNightMode, {}});
    EXPECT_EQ(night["active_now"], true);
    EXPECT_EQ(night["interval_secs"], 3600);
}
