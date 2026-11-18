#include "auth/FrameIdentity.hpp"
#include "support/Golden.hpp"
#include <gtest/gtest.h>
#include <map>
#include <sstream>

// gen-device-key.sh provisions the seed and prints the frame id the owner pairs; the board
// must derive the same id from the same seed.
TEST(IdentityContract, FrameIdMatchesTheProvisioningTool)
{
    std::map<std::string, std::string> v;
    std::istringstream in(golden::read(golden::hardware("identity.txt")));
    for (std::string line; std::getline(in, line);)
        if (const auto eq = line.find('='); eq != std::string::npos)
            v[line.substr(0, eq)] = line.substr(eq + 1);

    EXPECT_EQ(FrameIdentity::fingerprint(v.at("seed")), v.at("frame_id"));
}
