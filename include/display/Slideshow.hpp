#pragma once
#include <cstdint>
#include <string_view>

namespace slideshow
{
/// What the slideshow loop is busy with.
enum class Phase
{
    Stopped,   ///< the loop doesn't run
    Paused,    ///< the owner switched the slideshow off
    Empty,     ///< neither pictures nor samples to show
    Yielding,  ///< a sample waits for the panel, so a picture arriving meanwhile takes its place
    Painting,
    Holding,   ///< the frame stays up until its interval ends
    Recovering ///< pause after a step that failed
};

constexpr std::string_view name(const Phase phase)
{
    switch (phase)
    {
    case Phase::Stopped: return "Stopped";
    case Phase::Paused: return "Paused";
    case Phase::Empty: return "Empty";
    case Phase::Yielding: return "Yielding";
    case Phase::Painting: return "Painting";
    case Phase::Holding: return "Holding";
    case Phase::Recovering: return "Recovering";
    }
    return "?";
}

/// The loop may stop or fail anywhere. A paint always ends in holding its frame.
constexpr bool canMove(const Phase from, const Phase to)
{
    if (to == Phase::Stopped || to == Phase::Recovering)
        return true;
    switch (from)
    {
    case Phase::Stopped:
    case Phase::Recovering:
        return to != Phase::Holding;
    case Phase::Paused:
        return to == Phase::Empty || to == Phase::Yielding || to == Phase::Painting;
    case Phase::Empty:
        return to == Phase::Paused || to == Phase::Yielding || to == Phase::Painting;
    case Phase::Yielding:
        return to == Phase::Paused || to == Phase::Empty || to == Phase::Painting;
    case Phase::Painting:
        return to == Phase::Holding;
    case Phase::Holding:
        return to == Phase::Paused || to == Phase::Empty || to == Phase::Yielding || to == Phase::Painting;
    }
    return false;
}

enum class Content
{
    None,
    Sample,
    Picture
};

/// What arrived while the loop painted or waited.
struct Requests
{
    bool skip = false;
    bool newPicture = false;
    bool panelWiped = false;
    /// Picture to show next, 0 for none.
    int64_t showId = 0;
};

enum class Plan
{
    Pause,
    WaitForContent,
    ShowSample,
    ShowPicture
};

/// Pictures outrank samples, which only fill in while there are none.
constexpr Plan plan(const bool active, const bool hasPictures, const bool hasSamples)
{
    if (!active)
        return Plan::Pause;
    if (hasPictures)
        return Plan::ShowPicture;
    return hasSamples ? Plan::ShowSample : Plan::WaitForContent;
}

/// Whether what the panel holds gives way before its interval ends. A sample yields to
/// the first picture, except at night, when new pictures wait for the morning.
constexpr bool endsHold(const Content held, const Requests& requests, const bool active, const bool night)
{
    if (!active || requests.skip)
        return true;
    switch (held)
    {
    case Content::Picture: return requests.showId != 0;
    case Content::Sample: return requests.newPicture && !night;
    case Content::None: break;
    }
    return false;
}

enum class WipeResponse
{
    Ignore,
    LeaveToNext, ///< the next frame is due before the panel may refresh again
    Repaint
};

/// What to do about the frame on hold after the daily clear.
constexpr WipeResponse onPanelWiped(const bool panelBlank, const int64_t refreshAllowedInMs, const int64_t nextDueInMs)
{
    if (!panelBlank)
        return WipeResponse::Ignore;
    return refreshAllowedInMs >= nextDueInMs ? WipeResponse::LeaveToNext : WipeResponse::Repaint;
}
} // namespace slideshow
