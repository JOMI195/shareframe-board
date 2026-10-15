#include "dashboard/SlideshowImageHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "dashboard/Validation.hpp"
#include "ipc/IpcProtocol.hpp"
#include "settings/PlaybackOrder.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stb_image.h>

using namespace dashboard;

namespace
{
std::optional<std::string> readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::nullopt;
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

std::string imageContentType(const std::string& bytes)
{
    if (bytes.starts_with("\x89PNG"))
        return "image/png";
    return "image/jpeg";
}
} // namespace

SlideshowImageHandlers::SlideshowImageHandlers(IpcClient& ipc, IImageRepository& images)
    : ipc_(ipc), images_(images), logger_(spdlog::default_logger()->clone("SlideshowImageHandlers"))
{
}

ix::HttpResponsePtr SlideshowImageHandlers::handleList(const ix::HttpRequestPtr& /*req*/) const
{
    const auto order = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetPlaybackOrder, {}});
    if (!order)
    {
        logger_->error("Failed to query playback order via IPC");
        return errorResponse(500, "Internal Server Error", "Service unavailable");
    }

    const auto mode = playbackModeFromString(order->value("mode", "")).value_or(PlaybackMode::Chronological);
    const auto customOrder = order->value("custom_order", std::vector<int64_t>{});
    const auto images = images_.getAll();
    // A shuffle cycle shows as a custom order; pictures not yet in it come last.
    const auto sequence = mode == PlaybackMode::Shuffle
        ? playback::arrange(images, PlaybackMode::Custom, order->value("shuffle_order", std::vector<int64_t>{}))
        : playback::arrange(images, mode, customOrder);

    nlohmann::json list = nlohmann::json::array();
    for (size_t i = 0; i < sequence.size(); ++i)
    {
        const auto& img = *std::ranges::find(images, sequence[i], &Image::id);
        int width = 0, height = 0, comp = 0;
        if (!stbi_info(img.imagePath.c_str(), &width, &height, &comp))
            width = height = 0;
        std::error_code ec;
        const auto size = std::filesystem::file_size(img.imagePath, ec);
        list.push_back({
            {"id", img.id},
            {"sender", img.sender},
            {"received_at", img.receivedAt},
            {"expires_at", img.expiresAt},
            {"width", width},
            {"height", height},
            {"size_bytes", ec ? 0 : static_cast<int64_t>(size)},
            {"position", i + 1},
        });
    }

    return jsonResponse(200, "OK", {
        {"mode", toString(mode)},
        {"custom_order", customOrder},
        {"current_image_id", order->value("current_image_id", int64_t{0})},
        {"images", list},
    });
}

ix::HttpResponsePtr SlideshowImageHandlers::handleFile(const ix::HttpRequestPtr& /*req*/, const QueryParams& qp) const
{
    int64_t id = 0;
    try
    {
        const auto it = qp.find("id");
        if (it == qp.end())
            return errorResponse(400, "Bad Request", "id is required");
        size_t used = 0;
        id = std::stoll(it->second, &used);
        if (used != it->second.size())
            return errorResponse(400, "Bad Request", "id must be an integer");
    }
    catch (...) { return errorResponse(400, "Bad Request", "id must be an integer"); }

    const auto found = images_.getByIds({id});
    if (found.empty())
        return errorResponse(404, "Not Found", "Picture not found");

    auto bytes = readFile(found.front().imagePath);
    if (!bytes)
    {
        logger_->warn("Picture {} has no readable file: {}", id, found.front().imagePath.string());
        return errorResponse(404, "Not Found", "Picture file missing");
    }
    const auto type = imageContentType(*bytes);
    // Ids are never reused for another picture, so the bytes behind one never change.
    return binaryResponse(type, std::move(*bytes), "private, max-age=86400");
}

ix::HttpResponsePtr SlideshowImageHandlers::handleUpdateOrder(const ix::HttpRequestPtr& req) const
{
    nlohmann::json body;
    try { body = nlohmann::json::parse(req->body); }
    catch (...) { return errorResponse(400, "Bad Request", "Invalid JSON"); }

    const auto mode = body.value("mode", "");
    if (!Validation::isValidPlaybackMode(mode))
        return errorResponse(400, "Bad Request", "mode must be 'shuffle', 'chronological', 'newest_first' or 'custom'");

    std::vector<int64_t> customOrder;
    if (body.contains("custom_order"))
    {
        const auto parsed = Validation::parseCustomOrder(body["custom_order"]);
        if (!parsed)
            return errorResponse(400, "Bad Request", "custom_order must be a list of picture ids");
        customOrder = *parsed;
    }
    else if (const auto current = ipc_.sendAndReceive(IpcMessage{IpcMessageType::GetPlaybackOrder, {}}))
    {
        customOrder = current->value("custom_order", std::vector<int64_t>{});
    }

    if (!ipc_.send(IpcMessage{IpcMessageType::UpdatePlaybackOrder, {{"mode", mode}, {"custom_order", customOrder}}}))
    {
        logger_->error("Failed to send update_playback_order via IPC");
        return errorResponse(500, "Internal Server Error", "IPC error");
    }

    return jsonResponse(200, "OK", {{"mode", mode}, {"custom_order", customOrder}});
}
