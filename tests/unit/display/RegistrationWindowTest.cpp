#include "display/RegistrationWindow.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr int64_t kWindow = 600;
}

TEST(RegistrationWindow, ShowsFromTheFirstRefusalUntilTheWindowEnds)
{
    RegistrationWindow window;
    EXPECT_FALSE(window.required(0));

    window.report(false, 100, kWindow);
    EXPECT_TRUE(window.required(100));
    EXPECT_EQ(window.endsAt(100), 100 + kWindow);

    window.report(false, 500, kWindow);
    EXPECT_TRUE(window.required(100 + kWindow - 1)) << "a repeated refusal must not move the window";
    EXPECT_FALSE(window.required(100 + kWindow));
    EXPECT_EQ(window.endsAt(100 + kWindow), std::nullopt);

    window.report(false, 5000, kWindow);
    EXPECT_FALSE(window.required(5000)) << "never again until reboot";
}

TEST(RegistrationWindow, RegisteringHidesItForGood)
{
    RegistrationWindow window;
    window.report(false, 0, kWindow);
    window.report(true, 10, kWindow);
    EXPECT_FALSE(window.required(10));

    window.report(false, 20, kWindow);
    EXPECT_FALSE(window.required(20));
}

TEST(RegistrationWindow, ARegisteredFrameNeverShowsIt)
{
    RegistrationWindow window;
    window.report(true, 0, kWindow);
    window.report(false, 10, kWindow);
    EXPECT_FALSE(window.required(10));
}
