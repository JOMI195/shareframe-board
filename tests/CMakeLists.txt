include(GoogleTest)

add_library(shareframe-test-support STATIC support/main.cpp)
target_include_directories(shareframe-test-support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(shareframe-test-support PUBLIC
        SHAREFRAME_MIGRATIONS_DIR="${CMAKE_SOURCE_DIR}/migrations"
        SHAREFRAME_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
target_link_libraries(shareframe-test-support PUBLIC
        shareframe-websocket-lib
        shareframe-display-lib
        shareframe-dashboard-lib
        shareframe-heartbeat-lib
        shareframe-update-lib
        GTest::gmock)

file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS unit/*.cpp)
add_executable(shareframe-unit-tests ${UNIT_TEST_SOURCES})
target_link_libraries(shareframe-unit-tests PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,shareframe-test-support>)
gtest_discover_tests(shareframe-unit-tests PROPERTIES LABELS unit DISCOVERY_TIMEOUT 30)

file(GLOB_RECURSE INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS integration/*.cpp)
add_executable(shareframe-integration-tests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(shareframe-integration-tests PRIVATE $<LINK_LIBRARY:WHOLE_ARCHIVE,shareframe-test-support>)
gtest_discover_tests(shareframe-integration-tests PROPERTIES LABELS integration DISCOVERY_TIMEOUT 30)
