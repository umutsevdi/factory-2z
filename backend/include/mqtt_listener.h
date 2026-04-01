#pragma once

#include "util.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <mosquitto.h>

namespace f2z {

class Database;
class WebSocketServer;

bool verify_broker_process(int port);

using HandlerCallback
    = std::function<void(const std::string& topic, const std::string& payload,
        std::shared_ptr<Database> db, std::shared_ptr<WebSocketServer> ws)>;

class MqttListener {
public:
    static std::shared_ptr<MqttListener> create(const AppConfig& config,
        std::shared_ptr<Database> database,
        std::shared_ptr<WebSocketServer> ws_server,
        const std::map<std::string, HandlerCallback>& handlers);
    ~MqttListener();

    MqttListener(const MqttListener&)            = delete;
    MqttListener& operator=(const MqttListener&) = delete;

    [[nodiscard]] bool is_connected() const noexcept;

    void start_listening() noexcept;
    void stop_listening() noexcept;

private:
    static void on_connect(struct mosquitto* mosq, void* userdata, int rc);
    static void on_disconnect(struct mosquitto* mosq, void* userdata, int rc);
    static void on_message(struct mosquitto* mosq, void* userdata,
        const struct mosquitto_message* message);
    static void on_subscribe(struct mosquitto* mosq, void* userdata, int mid,
        int qos_count, const int* granted_qos);

    void handle_message(
        const std::string& topic, const std::string& payload) noexcept;

    [[nodiscard]] bool connect(int timeout_ms = 5000) noexcept;
    void disconnect() noexcept;
    [[nodiscard]] bool subscribe(
        const std::string& topic, int qos = 1) noexcept;
    void try_reconnect() noexcept;

    explicit MqttListener(const AppConfig& config);

    struct mosquitto* _mosq;
    int _broker_port;
    std::string _client_id;
    std::atomic<bool> _connected;
    std::atomic<bool> _listening;
    std::atomic<bool> _should_reconnect;
    std::atomic<bool> _reconnecting;
    std::chrono::milliseconds _reconnect_delay;
    std::chrono::milliseconds _max_reconnect_delay;
    std::thread _reconnect_thread;
    std::shared_ptr<Database> _database;
    std::shared_ptr<WebSocketServer> _ws_server;
    std::map<std::string, HandlerCallback> _handlers;
    std::vector<std::string> _subscribed_topics;
    std::mutex _connect_mutex;
    std::condition_variable _connect_cv;
};

} // namespace f2z
