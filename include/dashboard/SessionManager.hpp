#pragma once
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

class SessionManager
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto kSessionIdle = std::chrono::minutes(30);

    explicit SessionManager(std::function<Clock::time_point()> now = Clock::now) : now_(std::move(now)) {}

    /// Creates a new session and returns its ID (64-char hex string).
    std::string createSession();

    /// True while the session exists and has been used within kSessionIdle; a use extends it.
    [[nodiscard]] bool isValid(const std::string& sessionId);

    /// Removes a session (logout).
    void removeSession(const std::string& sessionId);

    [[nodiscard]] size_t count() const;

private:
    struct Session
    {
        Clock::time_point expiresAt;
    };

    std::function<Clock::time_point()> now_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, Session> sessions_;
};
