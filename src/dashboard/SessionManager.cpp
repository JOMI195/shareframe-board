#include "dashboard/SessionManager.hpp"
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

std::string SessionManager::createSession(const SessionOrigin origin)
{
    unsigned char buf[32];
    if (RAND_bytes(buf, sizeof(buf)) != 1)
        return {};

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : buf)
        oss << std::setw(2) << static_cast<int>(b);

    std::string id = oss.str();

    std::lock_guard lk(mtx_);
    const auto now = now_();
    std::erase_if(sessions_, [&](const auto& s) { return now >= s.second.expiresAt; });
    sessions_[id] = Session{now + kSessionIdle, origin};
    return id;
}

bool SessionManager::isValid(const std::string& sessionId)
{
    return origin(sessionId).has_value();
}

std::optional<SessionOrigin> SessionManager::origin(const std::string& sessionId)
{
    std::lock_guard lk(mtx_);
    auto it = sessions_.find(sessionId);
    const auto now = now_();
    if (it == sessions_.end() || now >= it->second.expiresAt)
        return std::nullopt;
    it->second.expiresAt = now + kSessionIdle;
    return it->second.origin;
}

void SessionManager::removeSession(const std::string& sessionId)
{
    std::lock_guard lk(mtx_);
    sessions_.erase(sessionId);
}

void SessionManager::removeAllExcept(const std::string& keep)
{
    std::lock_guard lk(mtx_);
    std::erase_if(sessions_, [&](const auto& s) { return s.first != keep; });
}

void SessionManager::removeOrigin(const SessionOrigin origin)
{
    std::lock_guard lk(mtx_);
    std::erase_if(sessions_, [&](const auto& s) { return s.second.origin == origin; });
}

size_t SessionManager::count() const
{
    std::lock_guard lk(mtx_);
    return sessions_.size();
}
