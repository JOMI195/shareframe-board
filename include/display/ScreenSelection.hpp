#pragma once
#include <string_view>

/// What the panel shows. Exactly one content task runs per content screen.
enum class Screen
{
    Boot,      ///< boot logo, no content task
    Switching, ///< the previous task stopped, the next one isn't running yet
    Slideshow,
    NightBlank,
    Registration,
    ApSetup,
    Status
};

constexpr std::string_view name(const Screen screen)
{
    switch (screen)
    {
    case Screen::Boot: return "Boot";
    case Screen::Switching: return "Switching";
    case Screen::Slideshow: return "Slideshow";
    case Screen::NightBlank: return "NightBlank";
    case Screen::Registration: return "Registration";
    case Screen::ApSetup: return "ApSetup";
    case Screen::Status: return "Status";
    }
    return "?";
}

constexpr bool isContent(const Screen screen)
{
    return screen != Screen::Boot && screen != Screen::Switching;
}

/// A content screen starts only after the previous one stopped, and nothing returns to Boot.
constexpr bool canMove(const Screen from, const Screen to)
{
    if (to == Screen::Switching)
        return from != Screen::Switching;
    return from == Screen::Switching && isContent(to);
}

struct ScreenInputs
{
    bool statusRequested = false;
    bool apSetupRequired = false;
    bool registrationRequired = false;
    bool nightBlank = false;
};

/// The first wish that holds wins.
constexpr Screen select(const ScreenInputs& in)
{
    if (in.statusRequested)
        return Screen::Status;
    if (in.apSetupRequired)
        return Screen::ApSetup;
    if (in.registrationRequired)
        return Screen::Registration;
    if (in.nightBlank)
        return Screen::NightBlank;
    return Screen::Slideshow;
}

enum class ScreenAction
{
    Keep,
    Restart, ///< same screen, new content
    Switch
};

struct ScreenDecision
{
    Screen target;
    ScreenAction action;
    /// Blank the panel before the target starts.
    bool clearFirst;
    bool operator==(const ScreenDecision&) const = default;
};

/// `shownStale` says the content the current screen painted is outdated or was wiped. A
/// stopped slideshow paints nothing, so the screen it follows is wiped first.
constexpr ScreenDecision decide(const Screen current, const ScreenInputs& in, const bool shownStale,
                                const bool slideshowActive)
{
    const Screen target = select(in);
    if (target == current)
        return {target, shownStale ? ScreenAction::Restart : ScreenAction::Keep, false};

    const bool leavesScreen = current != Screen::Boot && current != Screen::Slideshow;
    return {target, ScreenAction::Switch, leavesScreen && target == Screen::Slideshow && !slideshowActive};
}
