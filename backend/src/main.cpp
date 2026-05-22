#include "database.h"
#include "mqtt_listener.h"
#include "util.h"
#include "websocket_server.h"

#include <json/json.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

std::atomic<bool> g_running { true };

void signal_handler(int signal)
{
    std::puts("");
    L_INFO("Shut down request has been received.");
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

static std::string get_current_timestamp()
{
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

struct ThresholdConfig {
    double threshold;
    const char* severity;
};

static const std::unordered_map<std::string, ThresholdConfig> THRESHOLDS = {
    { "temperature", { 90, "warning" } },
    { "pressure", { 6, "warning" } },
    { "voltage", { 450, "warning" } },
    { "vibration", { 4.5, "warning" } },
    { "current", { 40, "warning" } },
    { "cycle_time", { 90, "warning" } },
    { "load_kg", { 400, "warning" } },
};

static std::atomic<uint64_t> g_warning_counter { 0 };

static void handle_sensor(const std::string& topic, const std::string& payload,
    std::shared_ptr<f2z::Database> db, std::shared_ptr<f2z::WebSocketServer> ws)
{
    if (!db || !ws || !ws->is_running()) {
        return;
    }

    auto [object_id, metric] = db->resolve_mqtt_topic(topic);
    if (object_id.empty() || metric.empty()) {
        L_DEBUG("Unmapped MQTT topic: {}", topic);
        return;
    }

    double value = 0;
    try {
        value = std::stod(payload);
    } catch (...) {
        L_WARN("Invalid numeric payload for {}: {}", topic, payload);
        return;
    }

    std::unordered_map<std::string, double> metrics = { { metric, value } };
    ws->send_telemetry(object_id, metrics);

    if (db) {
        db->insert_sensor_data(topic, payload);
    }

    auto it = THRESHOLDS.find(metric);
    if (it != THRESHOLDS.end() && value > it->second.threshold) {
        uint64_t ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
                             .count();
        std::string warning_id = std::format("{}-{}-{}-{}", object_id, metric,
            ts_ms, g_warning_counter.fetch_add(1));

        Json::Value warning;
        warning["type"]      = "warning";
        warning["id"]        = warning_id;
        warning["timestamp"] = get_current_timestamp();
        warning["severity"]  = it->second.severity;
        warning["message"]   = std::format(
            "{} exceeded {} (current {})", metric, it->second.threshold, value);
        warning["objectId"]  = object_id;
        warning["metric"]    = metric;
        warning["value"]     = value;
        warning["threshold"] = it->second.threshold;

        Json::StreamWriterBuilder writer_builder;
        writer_builder["indentation"] = "";
        std::string warning_json = Json::writeString(writer_builder, warning);

        ws->send_warning(warning_json);
    }
}

int main()
{
    L_INFO("Starting Factory 2Z Digital Twin Platform...");
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto config = f2z::load_config("config.json");
    if (!config) {
        L_ERROR("Failed to load configuration");
        return 1;
    }

    auto database = f2z::Database::create(*config);
    if (!database) {
        L_FATAL("Failed to create database instance");
    }
    if (!database->connect(*config)) {
        L_FATAL("Failed to connect to database");
    }

    if (!f2z::verify_broker_process(config->mqtt.broker_port)) {
        return 1;
    }

    auto ws_server = f2z::WebSocketServer::create(*config, database);
    if (!ws_server) {
        L_FATAL("Failed to start WebSocket server on port {}",
            config->websocket.port);
    }
    auto mqtt_listener = f2z::MqttListener::create(*config, database, ws_server,
        {
            { "factory/+/temperature", handle_sensor },
            { "factory/+/pressure", handle_sensor },
            { "factory/+/vibration", handle_sensor },
            { "factory/+/current", handle_sensor },
            { "factory/+/rpm", handle_sensor },
            { "factory/+/voltage", handle_sensor },
            { "factory/+/load_kg", handle_sensor },
            { "factory/+/cycle_time", handle_sensor },
            { "factory/+/flow_rate", handle_sensor },
        });

    if (!mqtt_listener) {
        L_FATAL("Error: Failed to connect to MQTT broker on localhost:{}. "
                "Application cannot run without MQTT support.",
            config->mqtt.broker_port);
    }

    L_INFO("Platform running. Press Ctrl+C to exit.");
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    L_INFO("Shutting down Factory 2Z...");
    ws_server->stop();
    mqtt_listener->stop_listening();
    return 0;
}
