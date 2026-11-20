#pragma once
#include <vector>
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

    /// A real 1x1 grey JPEG (159 bytes): the board checks a picture's format and size before storing it.
    static std::vector<unsigned char> jpegBytes()
    {
        std::vector<unsigned char> b = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0x01, 0x01, 0x00,
                                        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00};
        b.insert(b.end(), 64, 0xFF);
        b.insert(b.end(), {0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x11, 0x00});
        for (const unsigned char tableClass : {0x00, 0x10})
        {
            b.insert(b.end(), {0xFF, 0xC4, 0x00, 0x14, tableClass, 0x01});
            b.insert(b.end(), 16, 0x00);
        }
        b.insert(b.end(), {0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0x3F, 0xFF, 0xD9});
        return b;
    }

    static std::string jpegBase64()
    {
        const auto bytes = jpegBytes();
        return Base64::encode(bytes.data(), bytes.size());
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
