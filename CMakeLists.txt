cmake_minimum_required(VERSION 3.28)
project(shareframe-service VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
find_package(Threads REQUIRED)
add_library(external INTERFACE)

# EXCLUDE_FROM_ALL on each FetchContent_Declare suppresses the subproject's
# install() rules. Without this, `cmake --install` would also dump these
# deps' headers/libs into the rootfs (Buildroot installs them via its own
# packages where available; everything else is statically linked into).

# glaze
FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG v7.2.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(glaze)
target_link_libraries(external INTERFACE glaze::glaze)

# nlohmann_json
FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(json)
target_link_libraries(external INTERFACE nlohmann_json::nlohmann_json)

# spdlog
find_package(spdlog 1.17.0 QUIET)
if (NOT spdlog_FOUND)
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
            EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(spdlog)
endif ()
target_link_libraries(external INTERFACE spdlog::spdlog)

# IXWebSocket
set(USE_TLS ON CACHE BOOL "Enable TLS support in IXWebSocket" FORCE)
FetchContent_Declare(ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG v11.4.5
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(ixwebsocket)
target_link_libraries(external INTERFACE ixwebsocket)

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(external INTERFACE OpenSSL::Crypto)

# ARMv6 lacks native 64-bit atomics — GCC emits libatomic calls
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    target_link_libraries(external INTERFACE atomic)
endif ()

# SQLiteCpp
find_package(SQLiteCpp QUIET)
if (NOT SQLiteCpp_FOUND)
    FetchContent_Declare(
            sqlitecpp
            GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
            GIT_TAG 3.3.3
            GIT_SHALLOW TRUE
            EXCLUDE_FROM_ALL
    )
    set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sqlitecpp)
endif ()
target_link_libraries(external INTERFACE SQLiteCpp)

# nng (nanomsg-next-gen) — cross-process messaging (REQ/REP + PUB/SUB).
# Static, no tests/tools, no TLS (ipc:// unix sockets only).
set(NNG_TESTS OFF CACHE BOOL "" FORCE)
set(NNG_TOOLS OFF CACHE BOOL "" FORCE)
set(NNG_ENABLE_TLS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        nng
        GIT_REPOSITORY https://github.com/nanomsg/nng.git
        GIT_TAG v1.8.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(nng)
target_link_libraries(external INTERFACE nng)


add_subdirectory(external/waveshare)
add_subdirectory(external/stb)

# --- shareframe-common: shared static library ---
set(COMMON_SOURCES
        src/config/AppConfig.cpp
        src/config/ConfigLoader.cpp
        src/db/Database.cpp
        src/db/MigrationRunner.cpp
        src/logging/Logger.cpp
        src/net/HTTPClient.cpp
        src/auth/AuthTokenManager.cpp
        src/auth/Ed25519Auth.cpp
        src/auth/FrameIdentity.cpp
        src/auth/PasswordHash.cpp
        src/auth/TokenAuth.cpp
        src/util/Base64.cpp
        src/util/FileHash.cpp
        src/repository/TokenRepository.cpp
        src/repository/ImageRepository.cpp
        src/repository/SettingsRepository.cpp
        src/repository/DisplayMetricsRepository.cpp
        src/app/Bootstrap.cpp
        src/util/Subprocess.cpp
        src/task/Task.cpp
        src/task/PeriodicTask.cpp
        src/ipc/IpcClient.cpp
        src/ipc/NngRepServer.cpp
        src/ipc/EventPublisher.cpp
        src/ipc/EventSubscriber.cpp
)
add_library(shareframe-common STATIC ${COMMON_SOURCES})
target_include_directories(shareframe-common PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(shareframe-common PUBLIC external)

# --- shareframe-websocket: network + image ingest ---
set(WEBSOCKET_SOURCES
        apps/websocket/main.cpp
        src/net/WebsocketClient.cpp
        src/image/ImageManager.cpp
        src/task/ImageUpdate.cpp
        src/task/ImageCheck.cpp
        src/ipc/EventBridge.cpp
)
add_executable(shareframe-websocket ${WEBSOCKET_SOURCES})
target_link_libraries(shareframe-websocket PRIVATE shareframe-common)

# --- shareframe-display: e-paper rendering + slideshow ---
set(DISPLAY_SOURCES
        apps/display/main.cpp
        src/display/DisplayManager.cpp
        src/display/stb_impl.cpp
        src/task/DisplayImageLoop.cpp
        src/task/DisplayClear.cpp
        src/settings/RuntimeSettings.cpp
)
add_executable(shareframe-display ${DISPLAY_SOURCES})
target_link_libraries(shareframe-display PRIVATE shareframe-common waveshare_epd stb)

# --- shareframe-dashboard: dashboard process ---
set(DASHBOARD_SOURCES
        apps/dashboard/main.cpp
        src/dashboard/DashboardServer.cpp
        src/dashboard/SessionManager.cpp
        src/dashboard/WifiManager.cpp
        src/dashboard/WifiHandlers.cpp
        src/dashboard/FrameHandlers.cpp
        src/dashboard/SystemHandlers.cpp
        src/dashboard/ServiceHandlers.cpp
        src/dashboard/UpdateHandlers.cpp
)
add_executable(shareframe-dashboard ${DASHBOARD_SOURCES})
target_link_libraries(shareframe-dashboard PRIVATE shareframe-common)

# --- shareframe-heartbeat: heartbeat process ---
set(HEARTBEAT_SOURCES
        apps/heartbeat/main.cpp
        src/task/Heartbeat.cpp
)
add_executable(shareframe-heartbeat ${HEARTBEAT_SOURCES})
target_link_libraries(shareframe-heartbeat PRIVATE shareframe-common)

# --- shareframe-update: A/B image update service (RAUC + tryboot) ---
set(UPDATE_SOURCES
        apps/update/main.cpp
        src/update/UpdateManager.cpp
)
add_executable(shareframe-update ${UPDATE_SOURCES})
target_link_libraries(shareframe-update PRIVATE shareframe-common)

# --- shareframe-migrate: standalone DB migration oneshot entrypoint ---
# Thin wrapper over Database::init(cfg, true) (logic lives in shareframe-common).
# Run by the shareframe-db-migrate s6 oneshot before services start; also the
# dev DB setup tool. Links common only (no display/network/nng deps).
add_executable(shareframe-migrate apps/migrate/main.cpp)
target_link_libraries(shareframe-migrate PRIVATE shareframe-common)

set(CONFIG_FILES config.toml .env.secrets.toml config.dev.toml config.prod.toml VERSION.toml)
foreach (f ${CONFIG_FILES})
    configure_file(${CMAKE_SOURCE_DIR}/${f} ${CMAKE_BINARY_DIR}/${f} COPYONLY)
endforeach ()

file(GLOB_RECURSE ASSET_IMAGE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/image/*)
foreach (ASSET_FILE ${ASSET_IMAGE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR} ${ASSET_FILE})
    configure_file(
            ${ASSET_FILE}
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

# Install rules — used by Buildroot's cmake-package infra and by anyone who
# runs `cmake --install`. Host devs still work straight from the build dir.
# Only the SELECTED profile overlay is installed: SHAREFRAME_PROFILE (default
# prod) is passed via -D from the buildroot package (shareframe-board.mk), which
# in turn gets it from automation.sh. The app merges config.${profile}.toml on
# top of base config.toml at runtime, picking the profile from
# /etc/shareframe-profile (written by post-build.sh, exported by the s6 run
# scripts). .env.secrets.toml is NOT installed: per-device identity, provisioned
# on /data/shareframe/.
include(GNUInstallDirs)
install(TARGETS shareframe-websocket shareframe-display shareframe-dashboard shareframe-heartbeat shareframe-update shareframe-migrate
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NOT SHAREFRAME_PROFILE)
    set(SHAREFRAME_PROFILE prod)
endif ()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/config.${SHAREFRAME_PROFILE}.toml)
    message(FATAL_ERROR "SHAREFRAME_PROFILE='${SHAREFRAME_PROFILE}' but config.${SHAREFRAME_PROFILE}.toml does not exist")
endif ()

# SYSCONFDIR: use the FULL_ variant — on prefix=/usr it resolves to /etc
# absolute, which is what we want. The non-FULL_ variant is relative ("etc"),
# so install(DESTINATION ...) would put files in /usr/etc on Buildroot.
install(FILES
        ${CMAKE_SOURCE_DIR}/config.toml
        ${CMAKE_SOURCE_DIR}/config.${SHAREFRAME_PROFILE}.toml
        ${CMAKE_SOURCE_DIR}/VERSION.toml
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/shareframe)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/migrations/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/shareframe/migrations
        FILES_MATCHING PATTERN "*.sql")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/shareframe/assets)