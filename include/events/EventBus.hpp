#pragma once
#include "events/Messages.hpp"
#include <any>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>

class EventBus
{
public:
    ~EventBus() { stop(); }

    void start()
    {
        logger_ = spdlog::default_logger()->clone("EventBus");
        dispatchThread_ = std::jthread([this](const std::stop_token& st)
        {
            logger_->info("EventBus dispatch thread started");
            while (!st.stop_requested())
            {
                try
                {
                    waitAndProcess(st);
                }
                catch (const std::exception& e)
                {
                    logger_->error("EventBus dispatch failed: {}", e.what());
                }
                catch (...)
                {
                    logger_->error("EventBus dispatch failed with unknown exception");
                }
            }
            logger_->info("EventBus dispatch thread stopped");
        });
    }

    void stop()
    {
        if (!dispatchThread_.joinable())
            return;
        logger_->info("Stopping EventBus dispatch thread");
        dispatchThread_.request_stop();
        cv_.notify_all();
        dispatchThread_.join();
        logger_->info("EventBus dispatch thread stopped");
    }

    template <Topic T>
    void subscribe(std::function<void(const TopicMessage_t<T>&)> handler)
    {
        handlers_[static_cast<int>(T)].push_back(
            [h = std::move(handler)](const std::any& a)
            {
                h(std::any_cast<const TopicMessage_t<T>&>(a));
            }
        );
    }

    template <Topic T>
    void publish(TopicMessage_t<T> msg)
    {
        {
            std::lock_guard lk(mtx_);
            queue_.emplace_back(static_cast<int>(T), std::any(std::move(msg)));
        }
        cv_.notify_one();
    }

    void waitAndProcess(const std::stop_token& token)
    {
        std::vector<std::pair<int, std::any>> batch;
        {
            std::unique_lock lk(mtx_);
            cv_.wait(lk, token, [this] { return !queue_.empty(); });
            if (token.stop_requested())
                return;
            batch.swap(queue_);
        }
        for (auto& [topicId, payload] : batch)
        {
            if (auto it = handlers_.find(topicId); it != handlers_.end())
            {
                for (auto& handler : it->second)
                {
                    try
                    {
                        handler(payload);
                    }
                    catch (const std::exception& e)
                    {
                        spdlog::error("EventBus handler failed for topic {}: {}", topicId, e.what());
                    }
                    catch (...)
                    {
                        spdlog::error("EventBus handler failed for topic {} with unknown exception", topicId);
                    }
                }
            }
        }
    }

private:
    using ErasedHandler = std::function<void(const std::any&)>;

    std::mutex mtx_;
    std::condition_variable_any cv_;
    std::vector<std::pair<int, std::any>> queue_;
    std::unordered_map<int, std::vector<ErasedHandler>> handlers_;
    std::jthread dispatchThread_;
    std::shared_ptr<spdlog::logger> logger_;
};
