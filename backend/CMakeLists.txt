cmake_minimum_required(VERSION 3.16)
project(factory_2z VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit tests" OFF)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

# MQTT Client Library (Mosquitto)
include(FetchContent)

FetchContent_Declare(
    mosquitto
    GIT_REPOSITORY https://github.com/eclipse/mosquitto.git
    GIT_TAG v2.0.18
    GIT_SHALLOW TRUE
)

set(WITH_STATIC_LIB ON CACHE BOOL "Build static library" FORCE)
set(WITH_SHARED_LIB ON CACHE BOOL "Build shared library" FORCE)
set(WITH_CLIENTS ON CACHE BOOL "Build MQTT clients" FORCE)
set(WITH_APPS ON CACHE BOOL "Build applications" FORCE)
set(WITH_BROKER ON CACHE BOOL "Build MQTT broker" FORCE)
set(WITH_TLS OFF CACHE BOOL "Include TLS support" FORCE)
set(WITH_CJSON ON CACHE BOOL "Include cJSON support" FORCE)
set(WITH_EPOLL OFF CACHE BOOL "Use epoll" FORCE)
set(WITH_MEMORY_TRACKING OFF CACHE BOOL "Memory tracking" FORCE)
set(WITH_LIBWRAP OFF CACHE BOOL "Use libwrap" FORCE)
set(WITH_XTASNESS OFF CACHE BOOL "Include XTASNESS" FORCE)
set(WITH_DOCS OFF CACHE BOOL "Build documentation" FORCE)
set(WITH_TESTING OFF CACHE BOOL "Build testing" FORCE)

FetchContent_MakeAvailable(mosquitto)

# WebSocket Library (uWebSockets)
FetchContent_Declare(
    uSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
    GIT_TAG v0.8.8
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(uSockets_content)

file(GLOB_RECURSE USOCKETS_SOURCES 
    "${usockets_content_SOURCE_DIR}/src/*.c" 
    "${usockets_content_SOURCE_DIR}/src/*.cpp"
)
add_library(uSockets ${USOCKETS_SOURCES})
target_include_directories(uSockets PUBLIC ${usockets_content_SOURCE_DIR}/src)
target_compile_definitions(uSockets PRIVATE LIBUS_USE_EPOLL LIBUS_NO_SSL)
target_link_libraries(uSockets PUBLIC ZLIB::ZLIB Threads::Threads)

FetchContent_Declare(
    uWebSockets_content
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG v20.70.0
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(uWebSockets_content)
add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE ${uwebsockets_content_SOURCE_DIR}/src/)
target_link_libraries(uWebSockets INTERFACE uSockets)

# JSON library (jsoncpp)
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.6
)
FetchContent_MakeAvailable(jsoncpp)

# Database dependencies for sqlpp11 connectors
find_package(SQLite3 REQUIRED)
find_package(PostgreSQL REQUIRED)

# sqlpp11 - type-safe SQL library with SQLite3 and PostgreSQL connectors
FetchContent_Declare(
    sqlpp11
    GIT_REPOSITORY https://github.com/rbock/sqlpp11.git
    GIT_TAG 0.64
    GIT_SHALLOW TRUE
)

set(BUILD_SQLITE3_CONNECTOR ON CACHE BOOL "" FORCE)
set(BUILD_POSTGRESQL_CONNECTOR ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(DEPENDENCY_CHECK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sqlpp11)

set(SOURCES
    src/main.cpp
    src/mqtt_listener.cpp
    src/database.cpp
    src/websocket_server.cpp
    src/broker_manager.cpp
    src/util.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${mosquitto_SOURCE_DIR}/include
        ${mosquitto_BINARY_DIR}/include
        ${jsoncpp_SOURCE_DIR}/include
        ${uwebsockets_content_SOURCE_DIR}/src
)

set(LINK_LIBS
    Threads::Threads
    ZLIB::ZLIB
    libmosquitto
    jsoncpp_static
    sqlpp11::sqlpp11
    sqlpp11::sqlite3
    sqlpp11::postgresql
    uWebSockets
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})

# Add mosquitto source directory for header includes
target_include_directories(${PROJECT_NAME} PRIVATE ${mosquitto_SOURCE_DIR}/src)

# Copy mosquitto broker binary next to application binary
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mosquitto>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
