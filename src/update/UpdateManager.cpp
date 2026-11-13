#include "update/UpdateManager.hpp"
#include "auth/TokenAuth.hpp"
#include "ipc/StatusScreenKind.hpp"
#include "util/FileHash.hpp"
#include "util/Subprocess.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
std::string readFirstLine(const std::string& path)
{
    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    return line;
}
} // namespace

UpdateManager::UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager,
                             IpcClient& displayIpc, Paths paths)
    : cfg_(cfg), http_(http), authTokenManager_(authTokenManager), displayIpc_(displayIpc), paths_(std::move(paths)),
      logger_(spdlog::default_logger()->clone("UpdateManager")), phase_(UpdatePhase::Idle, logger_)
{
}

UpdateManager::UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager,
                             IpcClient& displayIpc)
    : UpdateManager(cfg, http, authTokenManager, displayIpc, Paths{})
{
}

UpdateManager::~UpdateManager() = default;

bool UpdateManager::isNewerThanInstalled(const std::string& candidate) const
{
    return isVersionNewer(candidate, cfg_.version);
}

bool UpdateManager::isVersionNewer(const std::string& candidate, const std::string& current)
{
    // numeric dot-segments compare; non-numeric versions (dev sha) never match
    static const std::regex semver(R"(^\d+(\.\d+)*$)");
    if (!std::regex_match(candidate, semver) || !std::regex_match(current, semver))
        return candidate != current && std::regex_match(candidate, semver);

    // Segments compare as digit strings so arbitrarily long numbers can't overflow.
    const auto trimZeros = [](std::string s)
    {
        const auto nz = s.find_first_not_of('0');
        return nz == std::string::npos ? std::string() : s.substr(nz);
    };
    std::istringstream a(candidate), b(current);
    std::string sa, sb;
    while (true)
    {
        const bool ha = static_cast<bool>(std::getline(a, sa, '.'));
        const bool hb = static_cast<bool>(std::getline(b, sb, '.'));
        if (!ha && !hb)
            return false;
        const std::string va = ha ? trimZeros(sa) : "";
        const std::string vb = hb ? trimZeros(sb) : "";
        if (va.size() != vb.size())
            return va.size() > vb.size();
        if (va != vb)
            return va > vb;
    }
}

HttpResponse UpdateManager::fetchLatestRelease() const
{
    HttpResponse bad;
    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authTokenManager_);
    if (authHeaders.empty())
    {
        bad.errorMsg = "Auth token unavailable";
        return bad;
    }
    HTTPClient::Headers headers(authHeaders.begin(), authHeaders.end());
    return http_.get(cfg_.httpBaseUrl() + cfg_.update.httpLatestUrl, headers);
}

std::optional<nlohmann::json> UpdateManager::_latestReleaseJson() const
{
    auto resp = fetchLatestRelease();
    if (!resp.ok())
        return std::nullopt;
    try
    {
        auto j = nlohmann::json::parse(resp.body);
        const auto stringOrAbsent = [&](const char* key) { return !j.contains(key) || j[key].is_string(); };
        if (!j.is_object() || !stringOrAbsent("version") || !stringOrAbsent("criticality"))
        {
            logger_->warn("Ignoring malformed latest-release response");
            return std::nullopt;
        }
        return j;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::string UpdateManager::_bootedSlot()
{
    const std::string cmdline = readFirstLine("/proc/cmdline");
    const auto pos = cmdline.find("rauc.slot=");
    return pos == std::string::npos ? "" : cmdline.substr(pos + 10, 1);
}

std::string UpdateManager::_pendingSlot() const
{
    return fs::exists(paths_.pendingFile) ? readFirstLine(paths_.pendingFile) : "";
}

std::string UpdateManager::_committedSlot() const
{
    auto r = Subprocess::run({paths_.trybootBackend, "get-primary"}, 10);
    if (r.exitCode != 0)
        return "";
    auto s = r.stdOut;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
}

bool UpdateManager::startUpdate(std::string& error)
{
    std::lock_guard lock(mutex_);
    if (isBusy(phase_.state()))
    {
        error = "Update already in progress";
        return false;
    }
    if (!_pendingSlot().empty())
    {
        error = "Previous update awaits confirmation; reboot or wait first";
        return false;
    }

    auto release = _latestReleaseJson();
    if (!release)
    {
        error = "Failed to fetch latest release from server";
        return false;
    }
    const std::string version = release->value("version", "");
    if (!isNewerThanInstalled(version))
    {
        error = "No newer version available";
        return false;
    }

    phase_.moveTo(UpdatePhase::Checking);
    error_.clear();
    targetVersion_ = version;
    worker_ = std::jthread([this, rel = *release](const std::stop_token&) { _worker(rel); });
    return true;
}

void UpdateManager::_fail(const std::string& error, const std::string& toVersion)
{
    logger_->error("Update to {} failed: {}", toVersion, error);
    {
        std::lock_guard lock(mutex_);
        phase_.moveTo(UpdatePhase::Failed);
        error_ = error;
    }
    Subprocess::run({"shareframe-update-history", "install-failed", error}, 10);
}

void UpdateManager::_worker(nlohmann::json release)
{
    const std::string version = release.value("version", "");
    const std::string url = release.value("download_url", "");
    if (url.empty())
    {
        _fail("Release has no download_url", version);
        return;
    }

    // staged meta: consumed by shareframe-update-history on commit/rollback
    std::error_code ec;
    fs::create_directories(fs::path(paths_.pendingMeta).parent_path(), ec);
    {
        std::ofstream meta(paths_.pendingMeta);
        meta << nlohmann::json{{"from_version", cfg_.version}, {"to_version", version}}.dump() << "\n";
    }

    const std::string bundle = paths_.cacheDir + "/shareframe-" + version + ".raucb";
    const std::string part = bundle + ".part";
    fs::create_directories(paths_.cacheDir, ec);
    // stale bundles from skipped/aborted updates would pile up on /data; this
    // version's partial download stays so the transfer resumes
    for (const auto& e : fs::directory_iterator(paths_.cacheDir, ec))
        if ((e.path().extension() == ".raucb" || e.path().extension() == ".part") && e.path() != part)
            fs::remove(e.path(), ec);

    {
        std::lock_guard lock(mutex_);
        phase_.moveTo(UpdatePhase::Downloading);
        downloadedBytes_ = 0;
        totalBytes_ = 0;
    }

    auto authHeaders = TokenAuth::buildTokenAuthHeaders(authTokenManager_);
    HTTPClient::Headers headers(authHeaders.begin(), authHeaders.end());

    logger_->info("Downloading update {} from {}", version, url);
    auto dl = http_.downloadToFile(url, part, headers,
                                   [this](const size_t received, const size_t total)
                                   {
                                       downloadedBytes_ = received;
                                       totalBytes_ = total;
                                   });
    if (!dl.ok())
    {
        _fail("Download failed: " + (dl.errorMsg.empty()
                  ? "HTTP " + std::to_string(dl.statusCode) : dl.errorMsg), version);
        return;
    }

    // transport integrity: server's checksum (set at upload) must match the
    // downloaded file; the RAUC signature check remains the security boundary
    std::string expected = release.value("checksum", "");
    std::transform(expected.begin(), expected.end(), expected.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (expected.empty())
    {
        logger_->warn("Server release has no checksum; skipping integrity check");
    }
    else if (const auto actual = FileHash::sha256File(part); actual != expected)
    {
        fs::remove(part, ec);
        _fail("Checksum mismatch: expected " + expected + ", got " + actual, version);
        return;
    }

    fs::rename(part, bundle, ec);
    if (ec)
    {
        _fail("Failed to finalize download: " + ec.message(), version);
        return;
    }

    {
        std::lock_guard lock(mutex_);
        phase_.moveTo(UpdatePhase::Installing);
    }
    logger_->info("Installing update {} via rauc", version);
    Subprocess::run({"shareframe-led", "updating"}, 5);
    displayIpc_.send({IpcMessageType::ShowStatusScreen,
                      {{"screen", toString(StatusScreenKind::Update)}, {"version", version}}});
    auto inst = Subprocess::run({"rauc", "install", bundle}, 1800);
    fs::remove(bundle, ec);
    if (inst.exitCode != 0)
    {
        Subprocess::run({"shareframe-led", "error"}, 5);
        displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
        _fail("rauc install failed: " + inst.stdErr, version);
        return;
    }
    Subprocess::run({"shareframe-led", "off"}, 5);

    {
        std::lock_guard lock(mutex_);
        phase_.moveTo(UpdatePhase::AwaitingReboot);
    }
    logger_->info("Update {} installed; rebooting into staged slot (tryboot)", version);
    std::ofstream(paths_.trybootFlag).put('\n');
    const auto reboot = Subprocess::run({"s6-linux-init-shutdown", "-r", "now"}, 10);
    if (reboot.exitCode == 0)
        return;

    // The slot stays staged, so the next reboot still boots into it.
    auto reason = reboot.stdErr;
    while (!reason.empty() && (reason.back() == '\n' || reason.back() == '\r'))
        reason.pop_back();
    displayIpc_.send({IpcMessageType::HideStatusScreen, {}});
    _fail("Installed, but the reboot failed: " + reason, version);
}

void UpdateManager::startPeriodicCheck()
{
    if (cfg_.update.checkIntervalSecs <= 0)
        return;
    checker_ = std::jthread(
        [this](const std::stop_token& st)
        {
            while (!st.stop_requested())
            {
                for (int i = 0; i < cfg_.update.checkIntervalSecs && !st.stop_requested(); ++i)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                if (st.stop_requested())
                    return;
                if (!_pendingSlot().empty())
                    continue; // never stack on an unconfirmed update
                auto release = _latestReleaseJson();
                if (!release)
                    continue;
                const std::string version = release->value("version", "");
                const std::string criticality = release->value("criticality", "");
                if (!isNewerThanInstalled(version))
                    continue;
                logger_->info("Update available: {} ({})", version, criticality);
                if (criticality == cfg_.update.autoInstallCriticality)
                {
                    std::string err;
                    if (!startUpdate(err))
                        logger_->warn("Auto-install of {} not started: {}", version, err);
                }
            }
        });
}

nlohmann::json UpdateManager::status() const
{
    UpdatePhase phase;
    std::string error, targetVersion;
    {
        std::lock_guard lock(mutex_);
        phase = phase_.state();
        error = error_;
        targetVersion = targetVersion_;
    }

    const auto pendingSlot = _pendingSlot();
    nlohmann::json st{
        {"phase", std::string(toString(phase))},
        {"error", error},
        {"target_version", targetVersion},
        {"current_version", cfg_.version},
        {"booted_slot", _bootedSlot()},
        {"committed_slot", _committedSlot()},
        {"pending_slot", pendingSlot},
        {"committed", pendingSlot.empty()},
        {"progress", -1},
    };

    if (phase == UpdatePhase::Downloading && totalBytes_ > 0)
        st["progress"] = static_cast<int>((downloadedBytes_ * 100) / totalBytes_);

    // last finished attempt (committed / rolled-back / install-failed)
    std::ifstream hist(paths_.historyFile);
    std::string line, last;
    while (std::getline(hist, line))
        if (!line.empty())
            last = line;
    if (!last.empty())
    {
        try
        {
            st["last_result"] = nlohmann::json::parse(last);
        }
        catch (...)
        {
        }
    }
    return st;
}

nlohmann::json UpdateManager::history() const
{
    auto entries = nlohmann::json::array();
    std::ifstream f(paths_.historyFile);
    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty())
            continue;
        try
        {
            entries.push_back(nlohmann::json::parse(line));
        }
        catch (...)
        {
        }
    }
    // newest first for the dashboard
    std::reverse(entries.begin(), entries.end());
    return entries;
}
