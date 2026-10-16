#include "display/PhotoRenderer.hpp"
#include "repository/DisplayMetricsRepository.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include "task/DisplayClear.hpp"
#include <fstream>
#include <gtest/gtest.h>

namespace
{
class DisplayClearTest : public ::testing::Test, protected ImageFixture
{
protected:
    DisplayClearTest()
    {
        cfg.display.mockDisplay = true;
        cfg.display.minRefreshSecs = 0;
        cfg.display.mockOutputDir = (db.dir() / "panel").string();
        const std::string image = std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-generic-shareframe.jpg";
        EXPECT_TRUE(display.displayBuffer(PhotoRenderer::render(image), image));
    }

    void TearDown() override
    {
        task.stop();
        bus.stop();
    }

    std::string state() const
    {
        std::ifstream meta(std::filesystem::path(cfg.display.mockOutputDir) / "panel.json");
        return nlohmann::json::parse(meta).value("state", "");
    }

    DisplayMetricsRepository metrics{db.get()};
    DisplayManager display{cfg, metrics};
    DisplayClear task{bus, cfg, display};
};
}

TEST_F(DisplayClearTest, ClearRequestBlanksThePanel)
{
    task.subscribe();
    task.start();
    bus.start();
    bus.publish<Topic::CLEAR_DISPLAY>(nlohmann::json::object());

    EXPECT_TRUE(waitFor([&] { return state() == "cleared"; }));
}

TEST_F(DisplayClearTest, RequestArrivingBeforeTheTaskWaitsIsNotLost)
{
    task.subscribe();
    Recorder<Topic::CLEAR_DISPLAY> delivered(bus);
    bus.start();
    bus.publish<Topic::CLEAR_DISPLAY>(nlohmann::json::object());
    ASSERT_TRUE(waitFor([&] { return delivered.size() == 1; }));

    task.start();
    EXPECT_TRUE(waitFor([&] { return state() == "cleared"; }));
}
