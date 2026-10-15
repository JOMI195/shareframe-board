#include "task/DisplayClear.hpp"
#include "events/Messages.hpp"
#include "util/DailyTime.hpp"
#include <chrono>
#include <ctime>

DisplayClear::DisplayClear(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "DisplayClear"), cfg_(cfg), display_(display)
{
}

void DisplayClear::subscribe()
{
    bus_.subscribe<Topic::CLEAR_DISPLAY>(
        [this](const nlohmann::json&) { _onClearDisplay(); });
}

void DisplayClear::start()
{
    const int waitSecs = _secsUntilNextTarget();
    logger_->info("Starting {} thread (next clear in {}s, target hour={})",
                  name_, waitSecs, cfg_.display.clearTargetHour);
    Task::start();
}

void DisplayClear::_onClearDisplay()
{
    std::lock_guard lk(mtx_);
    logger_->info("Clear display requested");
    clearRequested_ = true;
    cv_.notify_one();
}

void DisplayClear::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        const int waitSecs = _secsUntilNextTarget();
        logger_->info("Next display clear in {}s", waitSecs);

        {
            std::unique_lock lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(waitSecs), [&st, this]
            {
                return st.stop_requested() || clearRequested_;
            });
            clearRequested_ = false;
        }

        if (st.stop_requested())
            break;

        try
        {
            logger_->info("Clearing display");
            display_.clear();
        }
        catch (const std::exception& e)
        {
            logger_->error("DisplayClear execute() failed: {}", e.what());
        }
    }
}

int DisplayClear::_secsUntilNextTarget() const
{
    const std::time_t nowT = std::time(nullptr);
    std::tm local{};
    localtime_r(&nowT, &local);
    return daily::secsUntilHour(local, cfg_.display.clearTargetHour);
}
