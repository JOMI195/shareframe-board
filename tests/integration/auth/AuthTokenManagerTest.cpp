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
