#include "support/TempDir.hpp"
#include "util/WifiMode.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(WifiMode, HalfWrittenFileReadsAsConnecting)
{
    TempDir dir;
    const auto path = dir / "wifi-mode.json";
    std::ofstream(path) << R"({"mode":"ap","ssid":"","int)";

    EXPECT_EQ(WifiMode::read(path.string())["mode"], "connecting");
}
