#include "dashboard/SlideshowImageHandlers.hpp"
#include "ipc/NngRepServer.hpp"
#include "repository/ImageRepository.hpp"
#include "support/Http.hpp"
#include "support/TestDb.hpp"
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

TEST(SlideshowImageHandlers, AModeChangeKeepsTheCustomOrderWhenTheDisplayCannotTellIt)
{
    TestDb db;
    ImageRepository images{db.get()};
    const auto url = "ipc://" + (db.dir() / "display.sock").string();
    std::mutex mtx;
    std::vector<nlohmann::json> written;
    // A display busy painting: it misses the read of the current order but takes the write after it.
    NngRepServer display(url, [&](const IpcMessage& msg) -> nlohmann::json
    {
        if (msg.type == IpcMessageType::GetPlaybackOrder)
        {
            std::this_thread::sleep_for(2500ms);
            return {{"mode", "custom"}, {"custom_order", {3, 1, 2}}};
        }
        if (msg.type == IpcMessageType::UpdatePlaybackOrder)
        {
            std::lock_guard lk(mtx);
            written.push_back(msg.data);
        }
        return nlohmann::json::object();
    });
    display.start();
    IpcClient ipc(url);
    SlideshowImageHandlers handlers(ipc, images);

    const auto r = result(handlers.handleUpdateOrder(httpRequest("POST", "/api/frame/slideshow/order", {{"mode", "shuffle"}})));

    EXPECT_EQ(r.status, 503);
    // The stand-in is still answering the slow read; stopping it waits for that, so nothing is in flight.
    display.stop();
    std::lock_guard lk(mtx);
    EXPECT_TRUE(written.empty()) << written.front().dump();
}
