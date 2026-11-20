#pragma once
#include <string>

/// Short codes the panel shows to prove that someone stands at the frame.
namespace panel_code
{
/// A fresh 6-digit code from the system's randomness; empty if none is available.
std::string generate();

/// The code stored at `path`, created there (owner-only) if missing, so it survives a service restart.
std::string readOrCreate(const std::string& path);

/// The code stored at `path`; empty if there is none.
std::string read(const std::string& path);

/// "482913" as "482 913", easier to read off a panel.
std::string grouped(const std::string& code);
}
