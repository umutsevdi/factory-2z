#pragma once

#include <json/json.h>
#include <optional>
#include <source_location>
#include <string>

namespace f2z {
struct AppConfig {
    struct {
        std::string path;
    } database;
    struct {
        int port;
    } websocket;
    struct {
        int broker_port;
        std::string client_id;
    } mqtt;
};

std::optional<AppConfig> load_config(const std::string& path);
void log(const char* severity, FILE* target, const std::string& message,
    const std::source_location& loc = std::source_location::current());
} // namespace f2z

#define F_BOLD "\033[1m"
#define F_UNDERLINE "\033[4m"
#define F_RED "\033[31m"
#define F_GREEN "\033[32m"
#define F_BLUE "\033[34m"
#define F_INVERT "\033[7m"
#define F_RESET "\033[0m"
#define F_TEAL "\033[96m"
#define __LOG__(S, FP, ...) f2z::log(S, FP, std::format(__VA_ARGS__))
#define L_DEBUG(...) __LOG__(F_TEAL "DEBUG", stdout, __VA_ARGS__)
#define L_INFO(...) __LOG__(F_GREEN "INFO ", stdout, __VA_ARGS__)
#define L_WARN(...) __LOG__(F_GREEN "WARN", stdout, __VA_ARGS__)
#define L_ERROR(...) __LOG__(F_RED "ERROR", stderr, __VA_ARGS__)
#define L_FATAL(...)                                                           \
    __LOG__(F_RED "FATAL", stderr, __VA_ARGS__);                               \
    exit(1);
