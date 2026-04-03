#include "task/DisplayClear.hpp"
#include <chrono>
#include <ctime>

DisplayClear::DisplayClear(EventBus& bus, const AppConfig& cfg, DisplayManager& display)
    : Task(bus, "DisplayClear"), cfg_(cfg), display_(display)
{
}

void DisplayClear::start()
{
    const int waitSecs = _secsUntilNextTarget();
    logger_->info("Starting {} thread (next clear in {}s, target hour={})",
                  name_, waitSecs, cfg_.display.clearTargetHour);
    Task::start();
}

void DisplayClear::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        const int waitSecs = _secsUntilNextTarget();
        logger_->info("Next display clear in {}s", waitSecs);

        {
            std::unique_lock lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(waitSecs), [&st]
            {
                return st.stop_requested();
            });
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
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowT = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&nowT, &local);

    std::tm target = local;
    target.tm_hour = cfg_.display.clearTargetHour;
    target.tm_min = 0;
    target.tm_sec = 0;

    std::time_t targetT = std::mktime(&target);

    if (targetT <= nowT)
        targetT += 24 * 60 * 60;

    return static_cast<int>(targetT - nowT);
}
