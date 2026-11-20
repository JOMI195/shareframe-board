#pragma once
#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

/// Brute-force guard for every sign-in. Per client (nginx's X-Real-IP), escalating: 5 free failures,
/// then a lock that doubles from 30 s up to an hour. An attempt counts as failed until it succeeds, and
/// only one runs at a time, so parallel guesses neither slip through nor starve the single core.
class LoginThrottle
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr int kFreeFailures = 5;
    static constexpr auto kFirstLock = std::chrono::seconds(30);
    static constexpr auto kMaxLock = std::chrono::hours(1);

    class Attempt
    {
    public:
        Attempt(LoginThrottle* owner, std::string client, std::unique_lock<std::mutex> slot, int retryAfterSecs)
            : owner_(owner), client_(std::move(client)), slot_(std::move(slot)), retryAfterSecs_(retryAfterSecs)
        {
        }

        [[nodiscard]] bool admitted() const { return slot_.owns_lock(); }
        [[nodiscard]] int retryAfterSecs() const { return retryAfterSecs_; }
        void succeeded() const { owner_->_clear(client_); }

    private:
        LoginThrottle* owner_;
        std::string client_;
        std::unique_lock<std::mutex> slot_;
        int retryAfterSecs_;
    };

    explicit LoginThrottle(std::function<Clock::time_point()> now = Clock::now) : now_(std::move(now)) {}

    /// Admits one verification for `client`, or says how long to wait.
    [[nodiscard]] Attempt begin(const std::string& client)
    {
        std::unique_lock slot(verifying_, std::try_to_lock);
        if (!slot.owns_lock())
            return {this, client, {}, 1};

        std::lock_guard lk(mtx_);
        const auto now = now_();
        std::erase_if(clients_, [&](const auto& c) { return now - c.second.lastAttempt > kMaxLock; });
        auto& state = clients_[client];
        if (state.lockedUntil > now)
            return {this, client, {},
                    static_cast<int>(std::chrono::ceil<std::chrono::seconds>(state.lockedUntil - now).count())};

        state.lastAttempt = now;
        if (++state.failures >= kFreeFailures)
        {
            const auto lock = kFirstLock * (1LL << std::min(state.failures - kFreeFailures, 7));
            state.lockedUntil = now + std::min<Clock::duration>(lock, kMaxLock);
        }
        return {this, client, std::move(slot), 0};
    }

private:
    struct ClientState
    {
        int failures = 0;
        Clock::time_point lockedUntil{};
        Clock::time_point lastAttempt{};
    };

    void _clear(const std::string& client)
    {
        std::lock_guard lk(mtx_);
        clients_.erase(client);
    }

    std::function<Clock::time_point()> now_;
    std::mutex verifying_;
    std::mutex mtx_;
    std::unordered_map<std::string, ClientState> clients_;
};
