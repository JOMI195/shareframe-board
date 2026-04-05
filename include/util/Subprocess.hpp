#pragma once
#include "util/NonInstantiable.hpp"
#include <string>
#include <vector>

struct SubprocessResult
{
    int exitCode;
    std::string stdOut;
    std::string stdErr;
};

class Subprocess : NonInstantiable
{
public:
    static SubprocessResult run(const std::vector<std::string>& args, int timeoutSecs = 30);
};
