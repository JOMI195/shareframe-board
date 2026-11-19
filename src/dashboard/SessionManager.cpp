#include "dashboard/SessionManager.hpp"
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

std::string SessionManager::createSession()
{
    // Generate 32 random bytes → 64-char hex string
    unsigned char buf[32];
    RAND_bytes(buf, sizeof(buf));

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : buf)
        oss << std::setw(2) << static_cast<int>(b);

    std::string id = oss.str();

    std::lock_guard lk(mtx_);
    const auto now = now_();
    std::erase_if(sessions_, [&](const auto& s) { return now >= s.second.expiresAt; });
    sessions_[id] = Session{now + kSessionIdle};
    return id;
}

bool SessionManager::isValid(const std::string& sessionId)
{
    std::lock_guard lk(mtx_);
    auto it = sessions_.find(sessionId);
    const auto now = now_();
    if (it == sessions_.end() || now >= it->second.expiresAt)
        return false;
    it->second.expiresAt = now + kSessionIdle;
    return true;
}

void SessionManager::removeSession(const std::string& sessionId)
{
    std::lock_guard lk(mtx_);
    sessions_.erase(sessionId);
}

size_t SessionManager::count() const
{
    std::lock_guard lk(mtx_);
    return sessions_.size();
}
