#pragma once
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class SessionManager
{
public:
    /// Creates a new session and returns its ID (64-char hex string).
    std::string createSession();

    /// Returns true if session exists and has not expired.
    [[nodiscard]] bool isValid(const std::string& sessionId) const;

    /// Removes a session (logout).
    void removeSession(const std::string& sessionId);

private:
    using Clock = std::chrono::steady_clock;
    static constexpr auto kSessionMaxAge = std::chrono::minutes(30);

    struct Session
    {
        Clock::time_point expiresAt;
    };

    mutable std::mutex mtx_;
    std::unordered_map<std::string, Session> sessions_;
};
