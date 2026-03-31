cmake_minimum_required(VERSION 4.1)
project(shareframe-service VERSION 1.0.0 LANGUAGES CXX C)

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


add_subdirectory(external/waveshare)
add_subdirectory(external/stb)

# --- shareframe-common: shared static library ---
set(COMMON_SOURCES
    src/config/AppConfig.cpp
    src/config/ConfigLoader.cpp
    src/db/Database.cpp
    src/db/MigrationRunner.cpp
    src/logging/ConsoleLogger.cpp
    src/logging/FileLogger.cpp
    src/net/HTTPClient.cpp
    src/auth/AuthTokenManager.cpp
    src/auth/Ed25519Auth.cpp
    src/auth/TokenAuth.cpp
    src/auth/ServerSignedResponse.cpp
    src/util/Base64.cpp
    src/repository/TokenRepository.cpp
    src/repository/ImageRepository.cpp
    src/repository/SettingsRepository.cpp
    src/app/Bootstrap.cpp
)
add_library(shareframe-common STATIC ${COMMON_SOURCES})
target_include_directories(shareframe-common PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(shareframe-common PUBLIC vendor)

# --- shareframe-service: main process ---
set(SERVICE_SOURCES
    apps/service/main.cpp
    src/display/DisplayManager.cpp
    src/display/stb_impl.cpp
    src/image/ImageManager.cpp
    src/net/WebsocketClient.cpp
    src/task/Task.cpp
    src/task/PeriodicTask.cpp
    src/task/DisplayImageLoop.cpp
    src/task/DisplayClear.cpp
    src/task/ImageUpdate.cpp
    src/task/Heartbeat.cpp
    src/task/ConfigSender.cpp
    src/task/ImageCheck.cpp
    src/ipc/IpcServer.cpp
    src/settings/RuntimeSettings.cpp
)
add_executable(shareframe-service ${SERVICE_SOURCES})
target_link_libraries(shareframe-service PRIVATE shareframe-common waveshare_epd stb)

# --- shareframe-dashboard: dashboard process ---
set(DASHBOARD_SOURCES
    apps/dashboard/main.cpp
    src/dashboard/DashboardServer.cpp
    src/dashboard/SessionManager.cpp
    src/dashboard/IpcClient.cpp
)
add_executable(shareframe-dashboard ${DASHBOARD_SOURCES})
target_link_libraries(shareframe-dashboard PRIVATE shareframe-common)

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

configure_file(
        ${CMAKE_SOURCE_DIR}/VERSION.toml
        ${CMAKE_BINARY_DIR}/VERSION.toml
        COPYONLY
)

file(GLOB_RECURSE STATIC_IMAGE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/static/image/*)
foreach (STATIC_FILE ${STATIC_IMAGE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR} ${STATIC_FILE})
    configure_file(
            ${STATIC_FILE}
            ${CMAKE_BINARY_DIR}/${REL_PATH}
            COPYONLY
    )
endforeach ()

file(GLOB MIGRATION_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/migrations/*.sql)
foreach (MIGRATION_FILE ${MIGRATION_FILES})
    get_filename_component(MIGRATION_FILENAME ${MIGRATION_FILE} NAME)
    configure_file(
            ${MIGRATION_FILE}
            ${CMAKE_BINARY_DIR}/migrations/${MIGRATION_FILENAME}
            COPYONLY
    )
endforeach ()