#pragma once
#include "image/DefaultImages.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

/// The default images the frame shows while it has none of its own.
class DefaultImageHandlers
{
public:
    using QueryParams = std::unordered_map<std::string, std::string>;

    explicit DefaultImageHandlers(DefaultImages& pictures);

    ix::HttpResponsePtr handleList(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleFile(const ix::HttpRequestPtr& req, const QueryParams& qp) const;
    /// The body is the picture itself, JPEG or PNG.
    ix::HttpResponsePtr handleUpload(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleRemove(const ix::HttpRequestPtr& req) const;

private:
    DefaultImages& pictures_;
    std::shared_ptr<spdlog::logger> logger_;
};
