#include "repository/DisplayMetricsRepository.hpp"
#include "support/ImageFixture.hpp"
#include "support/Wait.hpp"
#include "task/DisplayClear.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(DisplayClear, ClearRequestBlanksThePanel)
{
    ImageFixture f;
    f.cfg.display.mockDisplay = true;
    f.cfg.display.minRefreshSecs = 0;
    f.cfg.display.mockOutputDir = (f.db.dir() / "panel").string();
    DisplayMetricsRepository metrics(f.db.get());
    DisplayManager display(f.cfg, metrics);
    ASSERT_TRUE(display.displayImage(std::string(SHAREFRAME_ASSETS_DIR) + "/image/loading/logo-frame-loading-shareframe.jpg"));

    DisplayClear task(f.bus, f.cfg, display);
    task.start();
    f.bus.start();
    f.bus.publish<Topic::CLEAR_DISPLAY>(nlohmann::json::object());

    const auto state = [&]
    {
        std::ifstream meta(std::filesystem::path(f.cfg.display.mockOutputDir) / "panel.json");
        return nlohmann::json::parse(meta).value("state", "");
    };
    EXPECT_TRUE(waitFor([&] { return state() == "cleared"; }));
    task.stop();
    f.bus.stop();
}
