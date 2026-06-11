#pragma once
#include <chrono>
#include <mutex>

// Global brute-force throttle for the offline password login: the dashboard
// serves a single device, so per-client tracking buys nothing (handlers don't
// see the client IP anyway). 5 failures lock all attempts for 30 s.
class LoginThrottle
{
public:
    /// Seconds until attempts are accepted again; 0 when unlocked.
    [[nodiscard]] int retryAfterSecs() const
    {
        std::lock_guard lock(mtx_);
        const auto now = Clock::now();
        if (lockedUntil_ <= now)
            return 0;
        return static_cast<int>(
            std::chrono::ceil<std::chrono::seconds>(lockedUntil_ - now).count());
    }

    void recordFailure()
    {
        std::lock_guard lock(mtx_);
        if (++failures_ >= kMaxFailures)
        {
            lockedUntil_ = Clock::now() + kLockDuration;
            failures_ = 0;
        }
    }

    void recordSuccess()
    {
        std::lock_guard lock(mtx_);
        failures_ = 0;
        lockedUntil_ = {};
    }

private:
    using Clock = std::chrono::steady_clock;
    static constexpr int kMaxFailures = 5;
    static constexpr auto kLockDuration = std::chrono::seconds(30);

    mutable std::mutex mtx_;
    int failures_ = 0;
    Clock::time_point lockedUntil_{};
};
