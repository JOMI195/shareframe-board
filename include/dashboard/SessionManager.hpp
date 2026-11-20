#pragma once
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

/// How the owner proved who they are when the session was opened.
enum class SessionOrigin
{
    Password,
    OneTimeCode,
    SetupCode
};

class SessionManager
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto kSessionIdle = std::chrono::minutes(30);

    explicit SessionManager(std::function<Clock::time_point()> now = Clock::now) : now_(std::move(now)) {}

    /// Creates a new session and returns its ID (64-char hex string); empty when no randomness is available.
    std::string createSession(SessionOrigin origin = SessionOrigin::Password);

    /// True while the session exists and has been used within kSessionIdle; a use extends it.
    [[nodiscard]] bool isValid(const std::string& sessionId);

    /// The origin of a valid session (a use extends it); nothing for an unknown or expired one.
    [[nodiscard]] std::optional<SessionOrigin> origin(const std::string& sessionId);

    /// Removes a session (logout).
    void removeSession(const std::string& sessionId);

    /// Ends every session except `keep`.
    void removeAllExcept(const std::string& keep);

    /// Ends every session opened in the given way.
    void removeOrigin(SessionOrigin origin);

    [[nodiscard]] size_t count() const;

private:
    struct Session
    {
        Clock::time_point expiresAt;
        SessionOrigin origin;
    };

    std::function<Clock::time_point()> now_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, Session> sessions_;
};
