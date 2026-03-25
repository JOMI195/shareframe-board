#include "task/Task.hpp"
#include <chrono>

Task::Task(EventBus& bus, std::string name)
    : bus_(bus), logger_(spdlog::default_logger()->clone(name)), name_(std::move(name))
{
}

Task::~Task()
{
    stop();
}

void Task::start()
{
    thread_ = std::jthread([this](std::stop_token st)
    {
        int consecutiveFailures = 0;
        int backoffSecs = 1;

        while (!st.stop_requested())
        {
            constexpr int kMaxBackoffSecs = 60;
            constexpr int kMaxConsecutiveFailures = 10;
            auto startTime = std::chrono::steady_clock::now();
            try
            {
                _run(st);
                break; // normal exit (stop requested)
            }
            catch (const std::exception& e)
            {
                logger_->error("{} thread failed: {}", name_, e.what());
            }
            catch (...)
            {
                logger_->critical("{} thread failed with unknown exception", name_);
            }

            if (st.stop_requested())
                break;

            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (constexpr int kStableThresholdSecs = 60; elapsed > std::chrono::seconds(kStableThresholdSecs))
            {
                consecutiveFailures = 0;
                backoffSecs = 1;
            }
            else
            {
                ++consecutiveFailures;
            }

            if (consecutiveFailures >= kMaxConsecutiveFailures)
            {
                logger_->critical("{} exceeded {} consecutive rapid failures, aborting process",
                                  name_, kMaxConsecutiveFailures);
                std::abort();
            }

            logger_->warn("Restarting {} in {}s (rapid failure {}/{})",
                          name_, backoffSecs, consecutiveFailures, kMaxConsecutiveFailures);

            std::unique_lock lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(backoffSecs),
                         [&st] { return st.stop_requested(); });

            backoffSecs = std::min(backoffSecs * 2, kMaxBackoffSecs);
        }
    });
}

void Task::stop()
{
    if (!thread_.joinable())
        return;

    logger_->info("Stopping {} thread", name_);
    thread_.request_stop();
    cv_.notify_all();
    thread_.join();
    logger_->info("{} thread stopped", name_);
}
