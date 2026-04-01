#pragma once

#include "util.h"

#include <App.h>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace f2z {

class Database;

class WebSocketServer {
public:
    static std::shared_ptr<WebSocketServer> create(
        const AppConfig& config, std::shared_ptr<Database> database);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&)            = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] int port() const noexcept;
    [[nodiscard]] std::size_t client_count() const noexcept;

    void broadcast(const std::string& message);
    void broadcast_json(const std::string& type, const std::string& topic,
        const std::string& payload);

private:
    explicit WebSocketServer(int port);
    struct PerSocketData { };

    using WebSocketType = uWS::WebSocket<false, true, PerSocketData>;

    void run_server(std::promise<void>& ready_promise);
    void handle_client_message(WebSocketType* ws, std::string_view message);
    void add_client(WebSocketType* ws);
    void remove_client(WebSocketType* ws);

    int _port;
    std::atomic<bool> _running { false };
    std::atomic<bool> _should_stop { false };
    std::thread _server_thread;
    std::unique_ptr<uWS::App> _app;

    std::vector<WebSocketType*> _clients;
    mutable std::mutex _clients_mutex;
    std::shared_ptr<Database> _database;
};

} // namespace f2z
