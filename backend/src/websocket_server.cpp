#include "websocket_server.h"
#include "database.h"
#include "util.h"

#include <chrono>
#include <ctime>
#include <future>
#include <iomanip>
#include <sstream>

#include <json/json.h>

namespace f2z {

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

WebSocketServer::WebSocketServer(int port)
    : _port(port)
    , _app(std::make_unique<uWS::App>())
{
}

std::shared_ptr<WebSocketServer> WebSocketServer::create(
    const AppConfig& config, std::shared_ptr<Database> database)
{
    auto server = std::shared_ptr<WebSocketServer>(
        new WebSocketServer(config.websocket.port));
    server->_database              = std::move(database);
    auto ready_promise             = std::make_unique<std::promise<void>>();
    std::future<void> ready_future = ready_promise->get_future();
    server->_server_thread         = std::thread(
        [server_ptr = server.get(), p = std::move(ready_promise)]() mutable {
            server_ptr->run_server(*p);
        });
    if (ready_future.wait_for(std::chrono::seconds(5))
        == std::future_status::timeout) {
        L_ERROR("WebSocket server startup timed out");
        server->_should_stop = true;
        if (server->_server_thread.joinable()) {
            server->_server_thread.join();
        }
        return nullptr;
    }
    try {
        ready_future.get();
    } catch (const std::exception& e) {
        L_ERROR("WebSocket server failed: {}", e.what());
        server->_should_stop = true;
        if (server->_server_thread.joinable()) {
            server->_server_thread.join();
        }
        return nullptr;
    }
    return server;
}

WebSocketServer::~WebSocketServer() { stop(); }

void WebSocketServer::start()
{
    if (_running) {
        L_INFO("WebSocket server already running");
        return;
    }

    _should_stop                   = false;
    auto ready_promise             = std::make_unique<std::promise<void>>();
    std::future<void> ready_future = ready_promise->get_future();
    _server_thread                 = std::thread(
        [this, p = std::move(ready_promise)]() mutable { run_server(*p); });

    if (ready_future.wait_for(std::chrono::seconds(5))
        == std::future_status::timeout) {
        L_ERROR("WebSocket server startup timed out");
        _should_stop = true;
        if (_server_thread.joinable()) {
            _server_thread.join();
        }
        return;
    }
    try {
        ready_future.get();
    } catch (const std::exception& e) {
        L_ERROR("WebSocket server failed: {}", e.what());
        _should_stop = true;
        if (_server_thread.joinable()) {
            _server_thread.join();
        }
    }
}

void WebSocketServer::stop()
{
    if (!_running) {
        return;
    }
    _should_stop = true;
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        for (auto* ws : _clients) {
            if (ws) {
                ws->close();
            }
        }
        _clients.clear();
    }

    if (_server_thread.joinable()) {
        _server_thread.join();
    }

    _running = false;
}

bool WebSocketServer::is_running() const noexcept { return _running; }

int WebSocketServer::port() const noexcept { return _port; }

std::size_t WebSocketServer::client_count() const noexcept
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    return _clients.size();
}

void WebSocketServer::broadcast(const std::string& message)
{
    if (!_running) {
        return;
    }

    std::vector<WebSocketType*> clients_copy;
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        clients_copy = _clients;
    }

    for (auto* ws : clients_copy) {
        if (ws) {
            ws->send(message, uWS::OpCode::TEXT);
        }
    }
}

void WebSocketServer::broadcast_json(const std::string& type,
    const std::string& topic, const std::string& payload)
{
    Json::Value root;
    root["type"]      = type;
    root["timestamp"] = get_current_timestamp();
    root["topic"]     = topic;

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value payload_json;
    std::string errs;

    if (reader->parse(payload.c_str(), payload.c_str() + payload.size(),
            &payload_json, &errs)) {
        root["payload"] = payload_json;
    } else {
        root["payload"] = payload;
    }

    Json::StreamWriterBuilder writer_builder;
    writer_builder["indentation"] = "";
    std::string json_str          = Json::writeString(writer_builder, root);

    broadcast(json_str);
}

void WebSocketServer::run_server(std::promise<void>& ready_promise)
{
    _app->ws<PerSocketData>("/*",
            { .open =
                    [this](auto* ws) {
                        add_client(ws);
                        L_INFO("WebSocket client connected. Total clients: {}",
                            client_count());
                    },
                .message =
                    [this](auto* ws, std::string_view message,
                        uWS::OpCode opCode) {
                        handle_client_message(ws, message);
                    },
                .close =
                    [this](auto* ws, int, std::string_view message) {
                        (void)message;
                        remove_client(ws);
                        L_INFO(
                            "WebSocket client disconnected. Total clients: {}",
                            client_count());
                    } })
        .get("/test",
            [](auto* res, auto* req) {
                (void)req;
                res->writeStatus("200 OK")->end("Hello from HTTP route");
            })
        .get("/api/device/:machine_id/config",
            [this](auto* res, auto* req) {
                std::string machine_id
                    = std::string(req->getParameter("machine_id"));
                std::string_view query = req->getQuery();
                std::string token;
                if (!query.empty()) {
                    size_t token_pos = query.find("token=");
                    if (token_pos != std::string_view::npos) {
                        size_t amp_pos            = query.find('&', token_pos);
                        std::string_view token_sv = query.substr(token_pos + 6,
                            amp_pos == std::string_view::npos
                                ? std::string_view::npos
                                : amp_pos - token_pos - 6);
                        token                     = token_sv;
                    }
                }
                if (token.empty()) {
                    res->writeStatus("401 Unauthorized")
                        ->end("{\"error\":\"Missing token\"}");
                    return;
                }
                if (!_database) {
                    res->writeStatus("500 Internal Server Error")
                        ->end("{\"error\":\"Service unavailable\"}");
                    return;
                }
                auto config_opt
                    = _database->get_device_config(machine_id, token);
                if (!config_opt) {
                    res->writeStatus("401 Unauthorized")
                        ->end("{\"error\":\"Invalid token or device not "
                              "found\"}");
                    return;
                }
                Json::StreamWriterBuilder writer_builder;
                writer_builder["indentation"] = "";
                std::string json_response
                    = Json::writeString(writer_builder, *config_opt);
                res->writeHeader("Content-Type", "application/json")
                    ->end(json_response);
            })
        .listen(_port,
            [this, &ready_promise](auto* listen_socket) {
                if (listen_socket) {
                    L_INFO("WebSocket server listening on port {}", _port);
                    _running = true;
                    ready_promise.set_value();
                } else {
                    L_ERROR("Failed to listen on port {}", _port);
                    _running = false;
                    ready_promise.set_value();
                }
            })
        .run();
}

void WebSocketServer::handle_client_message(
    WebSocketType* ws, std::string_view message)
{
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errs;

    if (reader->parse(
            message.data(), message.data() + message.size(), &root, &errs)) {
        std::string type = root.get("type", "").asString();

        if (type == "ping") {
            Json::Value response;
            response["type"]      = "pong";
            response["timestamp"] = get_current_timestamp();

            Json::StreamWriterBuilder writer_builder;
            writer_builder["indentation"] = "";
            std::string json_str = Json::writeString(writer_builder, response);

            ws->send(json_str, uWS::OpCode::TEXT);
        }
    }
}

void WebSocketServer::add_client(WebSocketType* ws)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    _clients.push_back(ws);
}

void WebSocketServer::remove_client(WebSocketType* ws)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    _clients.erase(
        std::remove(_clients.begin(), _clients.end(), ws), _clients.end());
}

} // namespace f2z
