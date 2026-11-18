#include "support/Golden.hpp"
#include "util/SysInfo.hpp"
#include <gtest/gtest.h>
#include <sstream>

// Every line shareframe-sysinfo prints arrives with its text intact: identifiers as strings,
// metrics as the same number, flags as booleans.
TEST(SysInfoContract, EveryKeyKeepsItsValue)
{
    const auto raw = golden::read(golden::hardware("sysinfo.txt"));
    const auto info = SysInfo::parse(raw);

    std::istringstream lines(raw);
    for (std::string line; std::getline(lines, line);)
    {
        const auto eq = line.find('=');
        const auto key = line.substr(0, eq), text = line.substr(eq + 1);
        const auto& v = info.at(key);
        if (v.is_string())
            EXPECT_EQ(v, text) << key;
        else if (v.is_boolean())
            EXPECT_EQ(v ? "true" : "false", text) << key;
        else
            EXPECT_DOUBLE_EQ(v.get<double>(), std::stod(text)) << key;
    }
    EXPECT_EQ(info.at("version"), "1.10");
    EXPECT_EQ(info.at("os_sha"), "0123456");
    EXPECT_TRUE(info.at("cpu_temp_celsius").is_number());
    EXPECT_TRUE(info.at("proc_display_threads").is_number());
    EXPECT_EQ(info.at("wlan_connected"), true);
}
