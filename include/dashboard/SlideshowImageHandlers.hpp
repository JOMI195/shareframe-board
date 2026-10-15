#pragma once
#include "ipc/IpcClient.hpp"
#include "repository/IImageRepository.hpp"
#include <ixwebsocket/IXHttpServer.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

/// The pictures the slideshow plays and their play order.
class SlideshowImageHandlers
{
public:
    using QueryParams = std::unordered_map<std::string, std::string>;

    SlideshowImageHandlers(IpcClient& ipc, IImageRepository& images);

    ix::HttpResponsePtr handleList(const ix::HttpRequestPtr& req) const;
    ix::HttpResponsePtr handleFile(const ix::HttpRequestPtr& req, const QueryParams& qp) const;
    ix::HttpResponsePtr handleUpdateOrder(const ix::HttpRequestPtr& req) const;

private:
    IpcClient& ipc_;
    IImageRepository& images_;
    std::shared_ptr<spdlog::logger> logger_;
};
