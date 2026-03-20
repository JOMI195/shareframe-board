cmake_minimum_required(VERSION 4.1)
project(shareframe-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
add_library(vendor INTERFACE)

# glaze
FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG v7.2.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)
target_link_libraries(vendor INTERFACE glaze::glaze)

# nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(vendor INTERFACE nlohmann_json::nlohmann_json)

# spdlog
find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif ()
target_link_libraries(vendor INTERFACE spdlog::spdlog)

# IXWebSocket
FetchContent_Declare(ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG v11.4.5
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(ixwebsocket)
target_link_libraries(vendor INTERFACE ixwebsocket)

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(vendor INTERFACE OpenSSL::Crypto)

# SQLiteCpp
FetchContent_Declare(
        sqlitecpp
        GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
        GIT_TAG 3.3.2
        GIT_SHALLOW TRUE
)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sqlitecpp)
target_link_libraries(vendor INTERFACE SQLiteCpp)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE vendor)

configure_file(
        ${CMAKE_SOURCE_DIR}/config.toml
        ${CMAKE_BINARY_DIR}/config.toml
        COPYONLY
)

configure_file(
        ${CMAKE_SOURCE_DIR}/.env.secrets.toml
        ${CMAKE_BINARY_DIR}/.env.secrets.toml
        COPYONLY
)

configure_file(
        ${CMAKE_SOURCE_DIR}/config.dev.toml
        ${CMAKE_BINARY_DIR}/config.dev.toml
        COPYONLY
)

configure_file(
        ${CMAKE_SOURCE_DIR}/config.prod.toml
        ${CMAKE_BINARY_DIR}/config.prod.toml
        COPYONLY
)

file(GLOB MIGRATION_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/migrations/*.sql)
foreach(MIGRATION_FILE ${MIGRATION_FILES})
    get_filename_component(MIGRATION_FILENAME ${MIGRATION_FILE} NAME)
    configure_file(
            ${MIGRATION_FILE}
            ${CMAKE_BINARY_DIR}/migrations/${MIGRATION_FILENAME}
            COPYONLY
    )
endforeach()