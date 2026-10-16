#include "net/DiagnosticsUploader.hpp"
#include "auth/AuthTokenManager.hpp"
#include "auth/TokenAuth.hpp"
#include "config/AppConfig.hpp"
#include "net/HTTPClient.hpp"
#include "util/Subprocess.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
constexpr int kCollectTimeoutSecs = 60;
}

DiagnosticsUploader::DiagnosticsUploader(const AppConfig& cfg, const HTTPClient& http, AuthTokenManager& auth)
    : cfg_(cfg), http_(http), auth_(auth), logger_(spdlog::default_logger()->clone("Diagnostics"))
{
}

DiagnosticsUploader::Result DiagnosticsUploader::collectAndUpload(const std::string& trigger,
                                                                  std::optional<int64_t> commandId) const
{
    if (std::filesystem::exists(cfg_.diagnostics.pendingPath))
        _uploadPending(std::nullopt);

    const auto res = Subprocess::run({"shareframe-diag"}, kCollectTimeoutSecs);
    if (res.exitCode != 0 || res.stdOut.empty())
        return {false, "Collecting diagnostics failed: " + res.stdErr};
    return _post(res.stdOut, trigger, commandId);
}

DiagnosticsUploader::Result DiagnosticsUploader::captureBeforeReboot(int64_t commandId) const
{
    const auto res = Subprocess::run({"shareframe-diag", "--out", cfg_.diagnostics.pendingPath}, kCollectTimeoutSecs);
    if (res.exitCode != 0)
        return {false, "Collecting diagnostics failed: " + res.stdErr};
    return _uploadPending(commandId);
}

DiagnosticsUploader::Result DiagnosticsUploader::_uploadPending(std::optional<int64_t> commandId) const
{
    std::ifstream in(cfg_.diagnostics.pendingPath, std::ios::binary);
    std::stringstream package;
    package << in.rdbuf();
    auto result = _post(package.str(), "pre-reboot", commandId);
    if (result.ok)
    {
        std::error_code ec;
        std::filesystem::remove(cfg_.diagnostics.pendingPath, ec);
    }
    return result;
}

DiagnosticsUploader::Result DiagnosticsUploader::_post(const std::string& package, const std::string& trigger,
                                                       std::optional<int64_t> commandId) const
{
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(auth_);
    if (authHeaders.empty())
        return {false, "No auth token available"};
    HTTPClient::Headers headers(authHeaders.begin(), authHeaders.end());
    headers["Content-Type"] = "application/gzip";

    std::string url = cfg_.httpBaseUrl() + cfg_.diagnostics.httpUrl + "?trigger=" + trigger;
    if (commandId)
        url += "&command_id=" + std::to_string(*commandId);

    const auto res = http_.post(url, package, headers);
    if (res.statusCode == 401)
        auth_.invalidate();
    if (!res.ok())
    {
        logger_->warn("Diagnostics upload failed: {} {}", res.statusCode, res.errorMsg);
        return {false, "Upload failed (" + std::to_string(res.statusCode) + ")"};
    }
    logger_->info("Diagnostics uploaded ({} bytes, trigger={})", package.size(), trigger);
    return {true, {}};
}
