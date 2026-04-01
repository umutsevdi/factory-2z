#include "database.h"
#include "mqtt_listener.h"
#include "util.h"
#include "websocket_server.h"

#include <atomic>
#include <csignal>
#include <string>
#include <thread>

std::atomic<bool> g_running { true };

void signal_handler(int signal)
{
    std::puts("");
    L_INFO("Shut down request has been received.");
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

static void handle_sensor(const std::string& topic, const std::string& payload,
    std::shared_ptr<f2z::Database> db, std::shared_ptr<f2z::WebSocketServer> ws)
{
    if (db) {
        db->insert_sensor_data(topic, payload);
    }
    if (ws && ws->is_running()) {
        ws->broadcast_json("sensor_update", topic, payload);
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
            { "factory/+/vibration", handle_sensor },
            { "factory/+/pressure", handle_sensor },
            { "factory/+/humidity", handle_sensor },
            { "factory/+/status", handle_sensor },
            { "factory/+/alerts", handle_sensor },
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
    ws_server->stop();
    mqtt_listener->stop_listening();

    L_INFO("Shutting down Factory 2Z...");

    return 0;
}
