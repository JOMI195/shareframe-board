#include "task/MissingImageCheck.hpp"
#include "events/Messages.hpp"
#include "events/Topic.hpp"
#include "net/WsProtocol.hpp"
#include <nlohmann/json.hpp>

MissingImageCheck::MissingImageCheck(EventBus& bus, ImageRepository& repo)
    : Task(bus, "MissingImageCheck"), repo_(repo)
{
}

void MissingImageCheck::start()
{
    bus_.subscribe<Topic::WS_CONNECTED>([this](const WsConnectedEvent&)
    {
        std::lock_guard lk(mtx_);
        checkRequested_ = true;
        cv_.notify_one();
    });

    Task::start();
}

void MissingImageCheck::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, st, [this] { return checkRequested_; });
            if (st.stop_requested())
                break;
            checkRequested_ = false;
        }

        _checkMissing();
    }
}

void MissingImageCheck::_checkMissing() const
{
    const auto ids = repo_.getAllIds();

    const nlohmann::json payload = {
        {"type", wsMessageTypeToString(WsMessageType::CheckMissingImages)},
        {"sent_image_ids", ids}
    };

    logger_->info("Checking for missing images, {} image IDs sent", ids.size());
    bus_.publish<Topic::WS_SEND>(payload);
}
