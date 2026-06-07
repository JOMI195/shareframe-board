#pragma once
#include <string>

#include "util/NonInstantiable.hpp"

class FrameIdentity : NonInstantiable
{
public:
    // Derive the readable frame id from the ed25519 private seed:
    //   base32(SHA-256(public_key)[:10]) grouped as XXXX-XXXX-XXXX-XXXX.
    // Must match the server's frames.keys.public_key_fingerprint.
    static std::string fingerprint(const std::string& base64PrivateKey);
};
