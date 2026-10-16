#include "net/HTTPClient.hpp"
#include "support/FlakyHttpServer.hpp"
#include "support/TempDir.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
std::string makeBody()
{
    std::string s(256 * 1024, '\0');
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(i * 131 % 251);
    return s;
}

std::string slurp(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

class HTTPClientDownloadTest : public ::testing::Test
{
protected:
    const std::string body = makeBody();
    FlakyHttpServer server{body};
    TempDir dir;
    const std::string dest = (dir / "bundle.part").string();
    HTTPClient http{5, 2};
};
}

TEST_F(HTTPClientDownloadTest, DroppedConnectionResumesWhereItStopped)
{
    server.cutAfter(100000, 1);

    const auto resp = http.downloadToFile(server.url(), dest);

    ASSERT_TRUE(resp.ok()) << resp.errorMsg;
    EXPECT_EQ(slurp(dest), body);
    EXPECT_EQ(server.ranges(), (std::vector<std::string>{"", "100000-"}));
}

TEST_F(HTTPClientDownloadTest, ExistingPartialFileIsContinued)
{
    std::ofstream(dest, std::ios::binary) << body.substr(0, 5000);

    const auto resp = http.downloadToFile(server.url(), dest);

    ASSERT_TRUE(resp.ok()) << resp.errorMsg;
    EXPECT_EQ(slurp(dest), body);
    EXPECT_EQ(server.ranges(), (std::vector<std::string>{"5000-"}));
}

TEST_F(HTTPClientDownloadTest, ServerIgnoringRangeRestartsWithoutDuplicatingBytes)
{
    server.ignoreRange();
    std::ofstream(dest, std::ios::binary) << body.substr(0, 5000);

    const auto resp = http.downloadToFile(server.url(), dest);

    ASSERT_TRUE(resp.ok()) << resp.errorMsg;
    EXPECT_EQ(slurp(dest), body);
    EXPECT_EQ(server.ranges(), (std::vector<std::string>{"5000-", ""}));
}

TEST_F(HTTPClientDownloadTest, ErrorAnswerOnResumeKeepsThePartialFile)
{
    server.cutAfter(100000, 1);
    server.refuseResumeWith(429);

    const auto resp = http.downloadToFile(server.url(), dest);

    EXPECT_EQ(resp.statusCode, 429);
    EXPECT_EQ(slurp(dest), body.substr(0, 100000));
}

TEST_F(HTTPClientDownloadTest, ProgressCountsTheResumedBytes)
{
    server.cutAfter(100000, 1);
    std::vector<size_t> received;
    size_t lastTotal = 0;

    const auto resp = http.downloadToFile(server.url(), dest, {},
                                          [&](const size_t got, const size_t total)
                                          {
                                              received.push_back(got);
                                              lastTotal = total;
                                          });

    ASSERT_TRUE(resp.ok()) << resp.errorMsg;
    EXPECT_TRUE(std::is_sorted(received.begin(), received.end()));
    EXPECT_EQ(received.back(), body.size());
    EXPECT_EQ(lastTotal, body.size());
}
