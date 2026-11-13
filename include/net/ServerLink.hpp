#pragma once
#include "net/WsCloseCodes.hpp"
#include <cstdint>
#include <string_view>

namespace ws
{
/// The websocket's standing with the server.
enum class Link
{
    Connecting,   ///< never connected, no refusal known
    Unregistered, ///< never connected, the server refuses the frame because nobody owns it
    Online,
    Offline       ///< was online, lost the connection
};

constexpr std::string_view name(const Link link)
{
    switch (link)
    {
    case Link::Connecting: return "Connecting";
    case Link::Unregistered: return "Unregistered";
    case Link::Online: return "Online";
    case Link::Offline: return "Offline";
    }
    return "?";
}

/// A frame that was online once never counts as unregistered again before a reboot, so a
/// later refusal doesn't bring the setup screen back over the photos.
constexpr bool canMove(const Link from, const Link to)
{
    switch (from)
    {
    case Link::Connecting: return to == Link::Unregistered || to == Link::Online;
    case Link::Unregistered: return to == Link::Online;
    case Link::Online: return to == Link::Offline;
    case Link::Offline: return to == Link::Online;
    }
    return false;
}

constexpr Link onOpened(Link)
{
    return Link::Online;
}

constexpr Link onClosed(const Link link)
{
    return link == Link::Online ? Link::Offline : link;
}

constexpr Link onRefusedAsUnowned(const Link link)
{
    return link == Link::Connecting ? Link::Unregistered : link;
}

/// An unregistered frame retries often, so it notices soon after the owner registers it.
constexpr uint32_t maxReconnectWaitMs(const Link link)
{
    return link == Link::Unregistered ? 30'000 : 180'000;
}

constexpr bool isAuthRejection(const int closeCode)
{
    return closeCode == CLOSE_AUTH_REJECTED || closeCode == CLOSE_TOKEN_REVOKED;
}

/// A rejected token is replaced at once; otherwise a streak of failed reconnects is
/// taken as a sign that it went stale.
constexpr bool refreshesToken(const int closeCode, const int failedReconnects)
{
    constexpr int kStreak = 5;
    return isAuthRejection(closeCode) || failedReconnects >= kStreak;
}
} // namespace ws
