#pragma once
#include <chrono>
#include <functional>
#include <mutex>
#include <string>

/// The rules for the reset code the panel shows: one code at a time, valid for 5 minutes and 5 tries,
/// single use. Three codes in a row that end without a reset (expired, cancelled, guessed wrong) lock
/// new codes for an hour, so nobody can keep the panel repainting or guess their way through.
class FactoryResetGuard
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto kValidity = std::chrono::minutes(5);
    static constexpr int kMaxTries = 5;
    static constexpr int kBurnedBeforeLock = 3;
    static constexpr auto kLock = std::chrono::hours(1);

    enum class IssueResult { Fresh, Pending, Locked };
    struct Issue
    {
        IssueResult result;
        std::string code; ///< only for Fresh: what the panel has to show
        int secondsLeft;  ///< validity of the code, or the remaining lock
    };

    enum class CheckResult { Accepted, Wrong, NoCode, Locked };
    struct Check
    {
        CheckResult result;
        int triesLeft = 0;
        int secondsLeft = 0;
    };

    explicit FactoryResetGuard(std::function<Clock::time_point()> now = Clock::now,
                               std::function<std::string()> newCode = {});

    [[nodiscard]] Issue issue();
    [[nodiscard]] Check verify(const std::string& code);
    /// Drops a pending code; true when there was one, so the panel has something to take back.
    bool cancel();

private:
    void _expire(Clock::time_point now);
    void _burn(Clock::time_point now);
    static int _secs(Clock::duration d);

    std::function<Clock::time_point()> now_;
    std::function<std::string()> newCode_;
    std::mutex mtx_;
    std::string code_;
    Clock::time_point expiresAt_{};
    int triesLeft_ = 0;
    int burned_ = 0;
    Clock::time_point lockedUntil_{};
};
