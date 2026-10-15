#include <cstdlib>
#include <gmock/gmock.h>
#include <spdlog/spdlog.h>

int main(int argc, char** argv)
{
    const char* level = std::getenv("SHAREFRAME_TEST_LOG");
    spdlog::set_level(level ? spdlog::level::from_str(level) : spdlog::level::off);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
