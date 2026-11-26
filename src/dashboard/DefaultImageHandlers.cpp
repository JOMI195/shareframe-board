#include "dashboard/DefaultImageHandlers.hpp"
#include "dashboard/ResponseUtil.hpp"
#include "image/PictureLimits.hpp"

using namespace dashboard;

namespace
{
nlohmann::json listJson(const std::vector<DefaultImages::Entry>& entries)
{
    nlohmann::json images = nlohmann::json::array();
    for (const auto& entry : entries)
        images.push_back({
            {"name", entry.name},
            {"width", entry.width},
            {"height", entry.height},
            {"size_bytes", entry.sizeBytes},
        });
    return {
        {"min", DefaultImages::kMin},
        {"max", DefaultImages::kMax},
        {"images", images},
    };
}
} // namespace

DefaultImageHandlers::DefaultImageHandlers(DefaultImages& pictures)
    : pictures_(pictures), logger_(spdlog::default_logger()->clone("DefaultImageHandlers"))
{
}

ix::HttpResponsePtr DefaultImageHandlers::handleList(const ix::HttpRequestPtr& /*req*/) const
{
    return jsonResponse(200, "OK", listJson(pictures_.list()));
}

ix::HttpResponsePtr DefaultImageHandlers::handleFile(const ix::HttpRequestPtr& /*req*/, const QueryParams& qp) const
{
    const auto it = qp.find("name");
    if (it == qp.end() || !DefaultImages::isValidName(it->second))
        return errorResponse(400, "Bad Request", "name is invalid");
    const auto path = pictures_.find(it->second);
    if (!path)
        return errorResponse(404, "Not Found", "Picture not found");
    auto bytes = readFile(*path);
    if (!bytes)
    {
        logger_->warn("Default image {} has no readable file", it->second);
        return errorResponse(404, "Not Found", "Picture file missing");
    }
    // Named after their content, so the bytes behind a name never change.
    return pictureResponse(std::move(*bytes), "private, max-age=31536000, immutable");
}

ix::HttpResponsePtr DefaultImageHandlers::handleUpload(const ix::HttpRequestPtr& req) const
{
    if (req->body.size() > picture::kMaxFileBytes)
        return errorResponse(413, "Payload Too Large", "Picture too large");
    if (req->body.empty())
        return errorResponse(400, "Bad Request", "Not a JPEG or PNG picture");

    const std::span bytes(reinterpret_cast<const uint8_t*>(req->body.data()), req->body.size());
    switch (pictures_.add(bytes))
    {
    using enum DefaultImages::AddResult;
    case Added: return jsonResponse(201, "Created", listJson(pictures_.list()));
    case Invalid: return errorResponse(400, "Bad Request", "Not a JPEG or PNG picture");
    case Duplicate: return errorResponse(409, "Conflict", "Picture already there");
    case TooSimilar: return errorResponse(409, "Conflict", "Picture too similar");
    case Full: return errorResponse(409, "Conflict", "Too many default images");
    case NoSpace: return errorResponse(507, "Insufficient Storage", "Not enough space");
    }
    return errorResponse(500, "Internal Server Error", "Internal error");
}

ix::HttpResponsePtr DefaultImageHandlers::handleRemove(const ix::HttpRequestPtr& req) const
{
    const auto body = nlohmann::json::parse(req->body, nullptr, false);
    if (!body.is_object() || !body.contains("name") || !body["name"].is_string())
        return errorResponse(400, "Bad Request", "name is required");
    const auto name = body["name"].get<std::string>();
    if (!DefaultImages::isValidName(name))
        return errorResponse(400, "Bad Request", "name is invalid");

    switch (pictures_.remove(name))
    {
    using enum DefaultImages::RemoveResult;
    case Removed: return jsonResponse(200, "OK", listJson(pictures_.list()));
    case NotFound: return errorResponse(404, "Not Found", "Picture not found");
    case AtMinimum: return errorResponse(409, "Conflict", "Too few default images");
    }
    return errorResponse(500, "Internal Server Error", "Internal error");
}
