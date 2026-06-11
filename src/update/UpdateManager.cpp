#include "update/UpdateManager.hpp"
#include "auth/TokenAuth.hpp"
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
constexpr auto kReleaseFile = "/etc/shareframe-release";
constexpr auto kPendingFile = "/data/rauc/pending";
constexpr auto kPendingMeta = "/data/rauc/pending-meta.json";
constexpr auto kHistoryFile = "/data/shareframe/update-history.jsonl";
constexpr auto kCacheDir = "/data/cache";
constexpr auto kTrybootFlag = "/run/tryboot";
constexpr auto kTrybootBackend = "/usr/lib/rauc/tryboot-backend";

std::string phaseName(UpdateManager::Phase p)
{
    switch (p)
    {
        case UpdateManager::Phase::Idle: return "idle";
        case UpdateManager::Phase::Checking: return "checking";
        case UpdateManager::Phase::Downloading: return "downloading";
        case UpdateManager::Phase::Installing: return "installing";
        case UpdateManager::Phase::AwaitingReboot: return "awaiting-reboot";
        case UpdateManager::Phase::Failed: return "failed";
    }
    return "unknown";
}

std::string readFirstLine(const std::string& path)
{
    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    return line;
}
} // namespace

UpdateManager::UpdateManager(const AppConfig& cfg, HTTPClient& http, AuthTokenManager& authTokenManager)
    : cfg_(cfg), http_(http), authTokenManager_(authTokenManager),
      logger_(spdlog::default_logger()->clone("UpdateManager"))
{
}

UpdateManager::~UpdateManager() = default;

std::string UpdateManager::currentVersion()
{
    std::ifstream f(kReleaseFile);
    std::string line;
    while (std::getline(f, line))
        if (line.rfind("VERSION=", 0) == 0)
            return line.substr(8);
    return "";
}

bool UpdateManager::isVersionNewer(const std::string& candidate, const std::string& current)
{
    // numeric dot-segments compare; non-numeric versions (dev sha) never match
    static const std::regex semver(R"(^\d+(\.\d+)*$)");
    if (!std::regex_match(candidate, semver) || !std::regex_match(current, semver))
        return candidate != current && std::regex_match(candidate, semver);

    std::istringstream a(candidate), b(current);
    std::string sa, sb;
    while (true)
    {
        const bool ha = static_cast<bool>(std::getline(a, sa, '.'));
        const bool hb = static_cast<bool>(std::getline(b, sb, '.'));
        if (!ha && !hb)
            return false;
        const long va = ha ? std::stol(sa) : 0;
        const long vb = hb ? std::stol(sb) : 0;
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
        return nlohmann::json::parse(resp.body);
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

std::string UpdateManager::_pendingSlot()
{
    return fs::exists(kPendingFile) ? readFirstLine(kPendingFile) : "";
}

std::string UpdateManager::_committedSlot()
{
    auto r = Subprocess::run({kTrybootBackend, "get-primary"}, 10);
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
    if (phase_ == Phase::Checking || phase_ == Phase::Downloading || phase_ == Phase::Installing
        || phase_ == Phase::AwaitingReboot)
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
    if (!isVersionNewer(version, currentVersion()))
    {
        error = "No newer version available";
        return false;
    }

    phase_ = Phase::Checking;
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
        phase_ = Phase::Failed;
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
    fs::create_directories("/data/rauc", ec);
    {
        std::ofstream meta(kPendingMeta);
        meta << nlohmann::json{{"from_version", currentVersion()}, {"to_version", version}}.dump() << "\n";
    }

    fs::create_directories(kCacheDir, ec);
    // stale bundles from skipped/aborted updates would pile up on /data
    for (const auto& e : fs::directory_iterator(kCacheDir, ec))
        if (e.path().extension() == ".raucb" || e.path().extension() == ".part")
            fs::remove(e.path(), ec);
    const std::string bundle = std::string(kCacheDir) + "/shareframe-" + version + ".raucb";
    const std::string part = bundle + ".part";

    {
        std::lock_guard lock(mutex_);
        phase_ = Phase::Downloading;
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
        phase_ = Phase::Installing;
    }
    logger_->info("Installing update {} via rauc", version);
    Subprocess::run({"shareframe-led", "updating"}, 5);
    auto inst = Subprocess::run({"rauc", "install", bundle}, 1800);
    fs::remove(bundle, ec);
    if (inst.exitCode != 0)
    {
        Subprocess::run({"shareframe-led", "error"}, 5);
        _fail("rauc install failed: " + inst.stdErr, version);
        return;
    }
    Subprocess::run({"shareframe-led", "off"}, 5);

    {
        std::lock_guard lock(mutex_);
        phase_ = Phase::AwaitingReboot;
    }
    logger_->info("Update {} installed; rebooting into staged slot (tryboot)", version);
    std::ofstream(kTrybootFlag).put('\n');
    Subprocess::run({"s6-linux-init-shutdown", "-r", "now"}, 10);
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
                if (!isVersionNewer(version, currentVersion()))
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
    std::lock_guard lock(mutex_);
    nlohmann::json st{
        {"phase", phaseName(phase_)},
        {"error", error_},
        {"target_version", targetVersion_},
        {"current_version", currentVersion()},
        {"booted_slot", _bootedSlot()},
        {"committed_slot", _committedSlot()},
        {"pending_slot", _pendingSlot()},
        {"progress", -1},
    };
    st["committed"] = !st["pending_slot"].get<std::string>().empty() ? false : true;

    if (phase_ == Phase::Downloading && totalBytes_ > 0)
        st["progress"] = static_cast<int>((downloadedBytes_ * 100) / totalBytes_);

    // last finished attempt (committed / rolled-back / install-failed)
    std::ifstream hist(kHistoryFile);
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
    std::ifstream f(kHistoryFile);
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
