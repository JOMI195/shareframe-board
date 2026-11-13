#include "display/Slideshow.hpp"
#include <array>
#include <gtest/gtest.h>
#include <set>
#include <utility>

using namespace slideshow;

namespace
{
constexpr std::array kPhases = {Phase::Stopped, Phase::Paused, Phase::Empty, Phase::Yielding,
                                Phase::Painting, Phase::Holding, Phase::Recovering};
}

TEST(Slideshow, PicturesOutrankSamplesAndASwitchedOffSlideshowShowsNeither)
{
    EXPECT_EQ(plan(false, true, true), Plan::Pause);
    EXPECT_EQ(plan(true, true, true), Plan::ShowPicture);
    EXPECT_EQ(plan(true, false, true), Plan::ShowSample);
    EXPECT_EQ(plan(true, false, false), Plan::WaitForContent);
}

TEST(Slideshow, OnlyTheListedMovesBetweenWorkingPhasesAreAllowed)
{
    const std::set<std::pair<Phase, Phase>> allowed = {
        {Phase::Paused, Phase::Empty}, {Phase::Paused, Phase::Yielding}, {Phase::Paused, Phase::Painting},
        {Phase::Empty, Phase::Paused}, {Phase::Empty, Phase::Yielding}, {Phase::Empty, Phase::Painting},
        {Phase::Yielding, Phase::Paused}, {Phase::Yielding, Phase::Empty}, {Phase::Yielding, Phase::Painting},
        {Phase::Painting, Phase::Holding},
        {Phase::Holding, Phase::Paused}, {Phase::Holding, Phase::Empty}, {Phase::Holding, Phase::Yielding},
        {Phase::Holding, Phase::Painting},
    };
    for (const auto from : kPhases)
    {
        for (const auto to : kPhases)
        {
            if (from == to)
                continue;
            const bool startsOver = (from == Phase::Stopped || from == Phase::Recovering) && to != Phase::Holding;
            const bool expected = to == Phase::Stopped || to == Phase::Recovering || startsOver
                || allowed.contains({from, to});
            EXPECT_EQ(canMove(from, to), expected) << name(from) << " -> " << name(to);
        }
    }
}

TEST(Slideshow, OnlyAPictureKeepsItsTurnWhenANewOneArrivesByDay)
{
    const Requests arrived{.newPicture = true};
    for (const auto held : {Content::None, Content::Sample})
    {
        EXPECT_TRUE(endsHold(held, arrived, true, false));
        EXPECT_FALSE(endsHold(held, arrived, true, true)) << "the night costs no extra refresh";
    }
    EXPECT_FALSE(endsHold(Content::Picture, arrived, true, false));
}

TEST(Slideshow, AShowNowRequestEndsAPictureEarly)
{
    EXPECT_TRUE(endsHold(Content::Picture, {.showId = 7}, true, true));
    EXPECT_FALSE(endsHold(Content::Picture, {}, true, false));
}

TEST(Slideshow, SkippingOrSwitchingOffEndsAnyHold)
{
    for (const auto held : {Content::None, Content::Sample, Content::Picture})
    {
        EXPECT_TRUE(endsHold(held, {.skip = true}, true, true));
        EXPECT_TRUE(endsHold(held, {}, false, false));
        EXPECT_FALSE(endsHold(held, {.panelWiped = true}, true, false)) << "a wiped panel keeps the frame's turn";
    }
}

TEST(Slideshow, AWipedFrameComesBackOnlyWhenThePanelMayRefreshBeforeTheNextOne)
{
    EXPECT_EQ(onPanelWiped(false, 0, 60'000), WipeResponse::Ignore);
    EXPECT_EQ(onPanelWiped(true, 30'000, 60'000), WipeResponse::Repaint);
    EXPECT_EQ(onPanelWiped(true, 60'000, 60'000), WipeResponse::LeaveToNext);
    EXPECT_EQ(onPanelWiped(true, 90'000, 60'000), WipeResponse::LeaveToNext);
}
