#include "task/PeriodicTask.hpp"

PeriodicTask::PeriodicTask(EventBus& bus, const AppConfig& cfg, std::string name)
    : Task(bus, std::move(name)), cfg_(cfg)
{
}

void PeriodicTask::start()
{
    logger_->info("Starting {} thread (interval={}s)", name_, intervalSecs());
    Task::start();
}

void PeriodicTask::_run(std::stop_token st)
{
    while (!st.stop_requested())
    {
        try
        {
            execute();
        }
        catch (const std::exception& e)
        {
            logger_->error("{} execute() failed: {}", name_, e.what());
        }
        catch (...)
        {
            logger_->error("{} execute() failed with unknown exception", name_);
        }

        std::unique_lock lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(intervalSecs()), [&st]
        {
            return st.stop_requested();
        });
    }
}
