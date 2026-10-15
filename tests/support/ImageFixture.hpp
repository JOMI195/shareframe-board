#pragma once
#include "TestDb.hpp"
#include "events/EventBus.hpp"
#include "image/ImageManager.hpp"
#include "repository/ImageRepository.hpp"
#include "util/Base64.hpp"
#include <mutex>

class ImageFixture
{
public:
    ImageFixture()
    {
        cfg.image.imageSavePath = (db.dir() / "images").string();
    }

    static std::string jpegBase64()
    {
        static const unsigned char bytes[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0xFF, 0xD9};
        return Base64::encode(bytes, sizeof(bytes));
    }

    TestDb db;
    AppConfig cfg;
    ImageRepository repo{db.get()};
    EventBus bus;
};

template <Topic T>
class Recorder
{
public:
    explicit Recorder(EventBus& bus)
    {
        bus.subscribe<T>([this](const TopicMessage_t<T>& msg)
        {
            std::lock_guard lk(mtx_);
            msgs_.push_back(msg);
        });
    }

    std::vector<TopicMessage_t<T>> get() const
    {
        std::lock_guard lk(mtx_);
        return msgs_;
    }

    size_t size() const { return get().size(); }

private:
    mutable std::mutex mtx_;
    std::vector<TopicMessage_t<T>> msgs_;
};
