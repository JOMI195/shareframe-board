#include "dashboard/FactoryResetGuard.hpp"
#include "auth/PasswordHash.hpp"
#include "util/PanelCode.hpp"

FactoryResetGuard::FactoryResetGuard(std::function<Clock::time_point()> now, std::function<std::string()> newCode)
    : now_(std::move(now)), newCode_(newCode ? std::move(newCode) : std::function<std::string()>(panel_code::generate))
{
}

FactoryResetGuard::Issue FactoryResetGuard::issue()
{
    std::lock_guard lk(mtx_);
    const auto now = now_();
    _expire(now);
    if (lockedUntil_ > now)
        return {IssueResult::Locked, {}, _secs(lockedUntil_ - now)};
    if (!code_.empty())
        return {IssueResult::Pending, {}, _secs(expiresAt_ - now)};

    code_ = newCode_();
    if (code_.empty())
        return {IssueResult::Locked, {}, 0};
    expiresAt_ = now + kValidity;
    triesLeft_ = kMaxTries;
    return {IssueResult::Fresh, code_, _secs(kValidity)};
}

FactoryResetGuard::Check FactoryResetGuard::verify(const std::string& code)
{
    std::lock_guard lk(mtx_);
    const auto now = now_();
    _expire(now);
    if (lockedUntil_ > now)
        return {CheckResult::Locked, 0, _secs(lockedUntil_ - now)};
    if (code_.empty())
        return {CheckResult::NoCode};
    if (PasswordHash::constantTimeEquals(code, code_))
    {
        code_.clear();
        burned_ = 0;
        return {CheckResult::Accepted};
    }
    if (--triesLeft_ <= 0)
    {
        _burn(now);
        return {CheckResult::Wrong, 0, 0};
    }
    return {CheckResult::Wrong, triesLeft_, _secs(expiresAt_ - now)};
}

bool FactoryResetGuard::cancel()
{
    std::lock_guard lk(mtx_);
    const auto now = now_();
    _expire(now);
    if (code_.empty())
        return false;
    _burn(now);
    return true;
}

void FactoryResetGuard::_expire(const Clock::time_point now)
{
    if (!code_.empty() && now >= expiresAt_)
        _burn(now);
}

void FactoryResetGuard::_burn(const Clock::time_point now)
{
    code_.clear();
    if (++burned_ >= kBurnedBeforeLock)
    {
        burned_ = 0;
        lockedUntil_ = now + kLock;
    }
}

int FactoryResetGuard::_secs(const Clock::duration d)
{
    return static_cast<int>(std::chrono::ceil<std::chrono::seconds>(d).count());
}
