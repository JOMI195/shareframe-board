#include "auth/AuthTokenManager.hpp"
#include "net/DiagnosticsUploader.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/PathShim.hpp"
#include "support/TestDb.hpp"
#include "support/TokenStore.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
constexpr auto kDiagnostics = "/api/frames/frame-diagnostics/";

class DiagnosticsUploaderTest : public ::testing::Test
{
protected:
    DiagnosticsUploaderTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.diagnostics.pendingPath = (db.dir() / "pre-reboot-diag.gz").string();
        storeValidToken(db.get());
        backend.on(std::string("POST ") + kDiagnostics, 201);
    }

    PathShim shims;
    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
    DiagnosticsUploader uploader{cfg, http, auth};
};
}

TEST_F(DiagnosticsUploaderTest, PostsTheCollectedPackageWithTrigger)
{
    shims.add("shareframe-diag", "printf 'GZ\\001fresh'");
    const auto result = uploader.collectAndUpload("owner");

    ASSERT_TRUE(result.ok) << result.error;
    const auto requests = backend.requests(kDiagnostics);
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0].body, std::string("GZ\001fresh"));
    EXPECT_EQ(requests[0].uri, std::string(kDiagnostics) + "?trigger=owner");
    EXPECT_EQ(requests[0].headers.at("Content-Type"), "application/gzip");
    EXPECT_EQ(requests[0].headers.at("Authorization"), "Frame-Access-Token test-token");
}

TEST_F(DiagnosticsUploaderTest, FailingCollectionDoesNotUpload)
{
    shims.add("shareframe-diag", "exit 1");
    EXPECT_FALSE(uploader.collectAndUpload("owner").ok);
    EXPECT_TRUE(backend.requests(kDiagnostics).empty());
}

TEST_F(DiagnosticsUploaderTest, PreRebootPackageIsDeletedOnlyAfterUpload)
{
    shims.add("shareframe-diag", "printf 'pending' > \"$2\"");
    backend.on(std::string("POST ") + kDiagnostics, 503);
    EXPECT_FALSE(uploader.captureBeforeReboot(42).ok);
    EXPECT_TRUE(std::filesystem::exists(cfg.diagnostics.pendingPath));
    EXPECT_EQ(backend.requests(kDiagnostics).back().uri, std::string(kDiagnostics) + "?trigger=pre-reboot&command_id=42");

    shims.add("shareframe-diag", "printf 'fresh'");
    backend.on(std::string("POST ") + kDiagnostics, 201);
    ASSERT_TRUE(uploader.collectAndUpload("admin", 7).ok);

    const auto requests = backend.requests(kDiagnostics);
    ASSERT_GE(requests.size(), 3u);
    EXPECT_EQ(requests[requests.size() - 2].body, "pending");
    EXPECT_EQ(requests.back().body, "fresh");
    EXPECT_EQ(requests.back().uri, std::string(kDiagnostics) + "?trigger=admin&command_id=7");
    EXPECT_FALSE(std::filesystem::exists(cfg.diagnostics.pendingPath));
}
