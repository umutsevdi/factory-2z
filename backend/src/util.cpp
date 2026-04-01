#include "util.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>

namespace f2z {
static std::chrono::time_point<std::chrono::steady_clock> app_start_time
    = std::chrono::steady_clock::now();

static std::string read_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::optional<AppConfig> load_config(const std::string& path)
{
    std::string content = read_file(path);
    if (content.empty()) {
        L_ERROR("Failed to read config file: {}", path);
        return std::nullopt;
    }

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errs;

    if (!reader->parse(
            content.c_str(), content.c_str() + content.size(), &root, &errs)) {
        L_ERROR("Failed to parse config file: {}", errs);
        return std::nullopt;
    }

    AppConfig config;

    if (!root.isMember("database") || !root["database"].isMember("type")
        || !root["database"].isMember("connection_string")) {
        L_ERROR("Config missing required database fields");
        return std::nullopt;
    }
    config.database.type = root["database"]["type"].asString();
    config.database.connection_string
        = root["database"]["connection_string"].asString();

    if (!root.isMember("websocket") || !root["websocket"].isMember("port")) {
        L_ERROR("Config missing required websocket.port field");
        return std::nullopt;
    }
    config.websocket.port = root["websocket"]["port"].asInt();

    if (!root.isMember("mqtt") || !root["mqtt"].isMember("broker_port")
        || !root["mqtt"].isMember("client_id")) {
        L_ERROR("Config missing required mqtt fields");
        return std::nullopt;
    }
    config.mqtt.broker_port = root["mqtt"]["broker_port"].asInt();
    config.mqtt.client_id   = root["mqtt"]["client_id"].asString();
    return config;
}

static std::string current_time()
{
    using namespace std::chrono;
    uint64_t total
        = duration_cast<milliseconds>(steady_clock::now() - app_start_time)
              .count();
    uint16_t hour = static_cast<uint16_t>(total / 3'600'000ULL); // 60*60*1000
    uint8_t min   = static_cast<uint8_t>((total % 3'600'000ULL) / 60'000ULL);
    uint8_t sec   = static_cast<uint8_t>((total % 60'000ULL) / 1'000ULL);
    uint8_t ms    = static_cast<uint8_t>(total % 1'000ULL / 10);
    if (hour == 0) {
        return std::format("{:02}:{:02}:{:02}", min, sec, ms);
    }
    return std::format("{:02}:{:02}:{:02}", hour, min, sec);
}

void log(const char* severity, FILE* target, const std::string& message,
    const std::source_location& loc)
{

    std::println(target,
        F_BLUE "[{}] " F_BOLD "{:<8}" F_RESET F_GREEN
               "|{:16}:{:03}:{:02}|" F_RESET " {}" F_RESET,
        current_time(), severity,
        std::filesystem::path { loc.file_name() }.filename().stem().c_str(),
        loc.line(), loc.column(), message);
}

} // namespace f2z
