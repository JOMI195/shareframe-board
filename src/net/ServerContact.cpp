#include "net/ServerContact.hpp"
#include <chrono>

namespace
{
constexpr auto KEY = "server_last_contact_at";

int64_t unixNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

ServerContact::ServerContact(SettingsRepository& repo, Clock now)
    : repo_(repo), now_(now ? std::move(now) : unixNow),
      logger_(spdlog::default_logger()->clone("ServerContact"))
{
    try
    {
        if (const auto stored = repo_.get(KEY))
            lastContact_ = lastPersisted_ = std::stoll(*stored);
    }
    catch (const std::exception& e)
    {
        logger_->warn("Ignoring stored last server contact: {}", e.what());
    }
}

void ServerContact::onOpen()
{
    std::lock_guard lk(mtx_);
    const auto now = now_();
    connected_ = true;
    connectedSince_ = lastContact_ = now;
    _persist(now);
}

void ServerContact::onClose()
{
    std::lock_guard lk(mtx_);
    if (!connected_)
        return;
    const auto now = now_();
    connected_ = false;
    lastContact_ = now;
    _persist(now);
}

void ServerContact::touch()
{
    std::lock_guard lk(mtx_);
    if (!connected_)
        return;
    lastContact_ = now_();
    if (lastContact_ - lastPersisted_ >= PERSIST_EVERY_SECS)
        _persist(lastContact_);
}

ServerConnection ServerContact::snapshot() const
{
    std::lock_guard lk(mtx_);
    return {connected_, connected_ ? connectedSince_ : lastContact_, lastContact_};
}

void ServerContact::_persist(const int64_t at)
{
    try
    {
        repo_.set(KEY, std::to_string(at));
        lastPersisted_ = at;
    }
    catch (const std::exception& e)
    {
        logger_->error("Failed to persist last server contact: {}", e.what());
    }
}
