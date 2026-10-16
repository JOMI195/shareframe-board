#pragma once
#include "net/ServerConnection.hpp"
#include "repository/SettingsRepository.hpp"
#include <functional>
#include <mutex>
#include <spdlog/spdlog.h>

/// Tracks when the frame last heard from the server. The timestamp survives a
/// restart so a frame that boots without network still knows how long it has
/// been cut off.
class ServerContact
{
public:
    using Clock = std::function<int64_t()>;

    // Caps SD writes while connected; a power cut loses at most this much.
    static constexpr int64_t PERSIST_EVERY_SECS = 3600;

    explicit ServerContact(SettingsRepository& repo, Clock now = {});

    void onOpen();
    void onClose();
    /// Any sign of life from the server while connected.
    void touch();

    [[nodiscard]] ServerConnection snapshot() const;

private:
    void _persist(int64_t at);

    SettingsRepository& repo_;
    Clock now_;
    std::shared_ptr<spdlog::logger> logger_;
    mutable std::mutex mtx_;
    bool connected_ = false;
    int64_t connectedSince_ = 0;
    int64_t lastContact_ = 0;
    int64_t lastPersisted_ = 0;
};
