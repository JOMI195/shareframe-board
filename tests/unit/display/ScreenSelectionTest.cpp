#include "display/ScreenSelection.hpp"
#include <array>
#include <gtest/gtest.h>

namespace
{
constexpr std::array kAll = {Screen::Boot, Screen::Switching, Screen::Slideshow, Screen::NightBlank,
                             Screen::Registration, Screen::ApSetup, Screen::Status};
}

TEST(ScreenSelection, HigherWishesOutrankLowerOnes)
{
    ScreenInputs in{.statusRequested = true, .apSetupRequired = true, .registrationRequired = true, .nightBlank = true};
    EXPECT_EQ(select(in), Screen::Status);
    in.statusRequested = false;
    EXPECT_EQ(select(in), Screen::ApSetup);
    in.apSetupRequired = false;
    EXPECT_EQ(select(in), Screen::Registration);
    in.registrationRequired = false;
    EXPECT_EQ(select(in), Screen::NightBlank);
    in.nightBlank = false;
    EXPECT_EQ(select(in), Screen::Slideshow);
}

TEST(ScreenSelection, EveryChangeOfScreenPassesThroughSwitching)
{
    for (const auto from : kAll)
    {
        for (const auto to : kAll)
        {
            const bool stops = to == Screen::Switching && from != Screen::Switching;
            const bool starts = from == Screen::Switching && isContent(to);
            EXPECT_EQ(canMove(from, to), stops || starts) << name(from) << " -> " << name(to);
        }
    }
}

TEST(ScreenSelection, KeepsAScreenThatIsStillWantedAndUpToDate)
{
    const ScreenInputs night{.nightBlank = true};
    EXPECT_EQ(decide(Screen::NightBlank, night, false, true),
              (ScreenDecision{Screen::NightBlank, ScreenAction::Keep, false}));
    EXPECT_EQ(decide(Screen::NightBlank, night, true, true),
              (ScreenDecision{Screen::NightBlank, ScreenAction::Restart, false}));
}

TEST(ScreenSelection, StoppedSlideshowGetsABlankPanelAfterAnotherScreen)
{
    const ScreenInputs none;
    EXPECT_TRUE(decide(Screen::ApSetup, none, false, false).clearFirst);
    EXPECT_FALSE(decide(Screen::ApSetup, none, false, true).clearFirst) << "a running slideshow paints over it";
    EXPECT_FALSE(decide(Screen::Boot, none, false, false).clearFirst) << "the boot logo stays up";
    EXPECT_FALSE(decide(Screen::ApSetup, {.statusRequested = true}, false, false).clearFirst);
}

TEST(ScreenSelection, AFailedSwitchIsTriedAgain)
{
    const auto decision = decide(Screen::Switching, {.apSetupRequired = true}, false, true);
    EXPECT_EQ(decision.action, ScreenAction::Switch);
    EXPECT_EQ(decision.target, Screen::ApSetup);
    EXPECT_TRUE(decide(Screen::Switching, {}, false, false).clearFirst) << "nobody knows what the panel shows";
}
