#include "auth/AuthTokenManager.hpp"
#include "auth/FrameIdentity.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/TestDb.hpp"
#include <gtest/gtest.h>

namespace
{
constexpr auto kSeed = "21c9Dcjx35psgsQau2FZR9dE1fB3qUOSNYcyUe5FqTE=";
constexpr auto kObtain = "POST /api/frames/obtain-frame-token/";

class AuthTokenManagerTest : public ::testing::Test
{
protected:
    AuthTokenManagerTest()
    {
        cfg.baseUrl = backend.baseUrl();
        cfg.secrets.ed25519PrivateKey = kSeed;
        cfg.frameId = FrameIdentity::fingerprint(kSeed);
    }

    FakeHttpServer backend;
    TestDb db;
    AppConfig cfg;
    TokenRepository tokens{db.get()};
    HTTPClient http{5, 5};
    AuthTokenManager auth{cfg, tokens, http};
};
}

TEST_F(AuthTokenManagerTest, UnownedFrameIsToldApartFromOtherRefusals)
{
    backend.on(kObtain, 403, R"({"detail":"Frame is not registered to a user.","code":"frame_unregistered"})");
    EXPECT_FALSE(auth.getOrFetchToken().has_value());
    EXPECT_TRUE(auth.isUnregistered());

    backend.on(kObtain, 403, R"({"detail":"Invalid authorization credentials.","code":"authentication_failed"})");
    EXPECT_FALSE(auth.getOrFetchToken().has_value());
    EXPECT_FALSE(auth.isUnregistered());
}

TEST_F(AuthTokenManagerTest, TokenAfterRegisteringClearsTheRefusal)
{
    backend.on(kObtain, 403, R"({"code":"frame_unregistered","claim_code":"K7M2QX9P"})");
    ASSERT_FALSE(auth.getOrFetchToken().has_value());

    backend.on(kObtain, 200, R"({"access_token":"t","expires_at":"2099-01-01T00:00:00Z"})");
    EXPECT_EQ(auth.getOrFetchToken(), "t");
    EXPECT_FALSE(auth.isUnregistered());
    EXPECT_EQ(auth.claimCode(), "");
}

TEST_F(AuthTokenManagerTest, RefusalHandsOverTheClaimCode)
{
    backend.on(kObtain, 403,
               R"({"code":"frame_unregistered","claim_code":"K7M2QX9P","claim_expires_at":"2099-01-01T00:00:00Z"})");
    ASSERT_FALSE(auth.getOrFetchToken().has_value());

    EXPECT_EQ(auth.claimCode(), "K7M2QX9P");
}

TEST_F(AuthTokenManagerTest, ServerWithoutClaimCodesLeavesItEmpty)
{
    backend.on(kObtain, 403, R"({"code":"frame_unregistered"})");
    ASSERT_FALSE(auth.getOrFetchToken().has_value());

    EXPECT_TRUE(auth.isUnregistered());
    EXPECT_EQ(auth.claimCode(), "");
}

namespace
{
constexpr auto kVerify = "POST /api/frames/verify-frame-auth-token/";
const std::string kFresh = R"({"access_token":"fresh-token","expires_at":"2099-01-01T00:00:00"})";
}

// Every heartbeat and picture fetch asks for the token: a valid one must not cost a request.
TEST_F(AuthTokenManagerTest, AValidCachedTokenIsUsedWithoutAskingTheServer)
{
    tokens.save({"cached-token", std::time(nullptr) + 3600});
    EXPECT_EQ(auth.getOrFetchToken(), "cached-token");
    EXPECT_TRUE(backend.requests().empty());
}

TEST_F(AuthTokenManagerTest, AnExpiredTokenTheServerStillAcceptsIsKept)
{
    tokens.save({"old-token", std::time(nullptr) - 10});
    backend.on(kVerify, 200);
    EXPECT_EQ(auth.getOrFetchToken(), "old-token");
    EXPECT_TRUE(backend.requests("/api/frames/obtain-frame-token/").empty());
}

TEST_F(AuthTokenManagerTest, AnExpiredTokenTheServerRejectsIsReplaced)
{
    tokens.save({"old-token", std::time(nullptr) - 10});
    backend.on(kVerify, 401);
    backend.on(kObtain, 200, kFresh);
    EXPECT_EQ(auth.getOrFetchToken(), "fresh-token");
    EXPECT_EQ(tokens.get()->value, "fresh-token");
}

TEST_F(AuthTokenManagerTest, InvalidateMakesTheNextCallFetchAFreshToken)
{
    tokens.save({"revoked-token", std::time(nullptr) + 3600});
    backend.on(kObtain, 200, kFresh);
    auth.invalidate();
    EXPECT_EQ(auth.getOrFetchToken(), "fresh-token");
}

// The server's answer is trusted only when it is a token: anything else leaves no token.
TEST_F(AuthTokenManagerTest, AnUnusableAnswerGivesNoToken)
{
    backend.on(kObtain, 200, R"({"access_token":"t","expires_at":"next tuesday"})");
    EXPECT_FALSE(auth.getOrFetchToken());
    EXPECT_FALSE(tokens.get());
    backend.on(kObtain, 500);
    EXPECT_FALSE(auth.getOrFetchToken());
    EXPECT_FALSE(auth.isUnregistered());
}
