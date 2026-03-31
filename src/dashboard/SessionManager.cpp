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
    sessions_[id] = Session{Clock::now() + kSessionMaxAge};
    return id;
}

bool SessionManager::isValid(const std::string& sessionId) const
{
    std::lock_guard lk(mtx_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end())
        return false;
    return Clock::now() < it->second.expiresAt;
}

void SessionManager::removeSession(const std::string& sessionId)
{
    std::lock_guard lk(mtx_);
    sessions_.erase(sessionId);
}
