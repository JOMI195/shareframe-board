#include <atomic>
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <ixwebsocket/IXDNSLookup.h>
#include <netdb.h>

namespace
{
std::atomic<int> nullFrees{0};
}

// musl's freeaddrinfo dereferences its argument, glibc's ignores NULL. Interposing
// here makes a NULL free visible on the glibc test host.
extern "C" void freeaddrinfo(addrinfo* res)
{
    if (!res)
    {
        ++nullFrees;
        return;
    }
    static const auto real = reinterpret_cast<void (*)(addrinfo*)>(dlsym(RTLD_NEXT, "freeaddrinfo"));
    real(res);
}

TEST(DnsLookupTest, FailedLookupDoesNotFreeNull)
{
    nullFrees = 0;
    {
        const auto lookup = std::make_shared<ix::DNSLookup>("nonexistent.invalid", 443);
        std::string errMsg;
        const auto res = lookup->resolve(errMsg, [] { return false; }, false);
        EXPECT_EQ(res, nullptr);
    }
    EXPECT_EQ(nullFrees, 0);
}
