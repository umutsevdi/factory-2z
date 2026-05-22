#include "mqtt_listener.h"
#include "database.h"
#include "util.h"
#include "websocket_server.h"

#include <mosquitto.h>

namespace f2z {

MqttListener::MqttListener(const AppConfig& config)
    : _mosq(nullptr)
    , _broker_port(config.mqtt.broker_port)
    , _client_id(config.mqtt.client_id)
    , _connected(false)
    , _listening(false)
    , _should_reconnect(false)
    , _reconnecting(false)
    , _reconnect_delay(1000)
    , _max_reconnect_delay(30000)
{
    mosquitto_lib_init();
    _mosq = mosquitto_new(_client_id.c_str(), true, this);

    if (_mosq) {
        mosquitto_connect_callback_set(_mosq, on_connect);
        mosquitto_disconnect_callback_set(_mosq, on_disconnect);
        mosquitto_message_callback_set(_mosq, on_message);
        mosquitto_subscribe_callback_set(_mosq, on_subscribe);
    }
}

std::shared_ptr<MqttListener> MqttListener::create(const AppConfig& config,
    std::shared_ptr<Database> database,
    std::shared_ptr<WebSocketServer> ws_server,
    const std::map<std::string, HandlerCallback>& handlers)
{
    auto listener = std::shared_ptr<MqttListener>(new MqttListener(config));
    listener->_database         = database;
    listener->_ws_server        = ws_server;
    listener->_handlers         = handlers;
    listener->_should_reconnect = true;
    if (!listener->connect(5000)) {
        return nullptr;
    }
    for (const auto& [topic, _] : handlers) {
        if (!listener->subscribe(topic, 1)) {
            L_ERROR("Failed to subscribe to {}", topic);
        }
    }
    listener->start_listening();
    return listener;
}

MqttListener::~MqttListener()
{
    _should_reconnect = false;
    stop_listening();
    disconnect();

    if (_reconnect_thread.joinable()) {
        _reconnect_thread.join();
    }

    if (_mosq) {
        mosquitto_destroy(_mosq);
        mosquitto_lib_cleanup();
    }
}

bool MqttListener::connect(int timeout_ms) noexcept
{
    if (!_mosq) {
        L_ERROR("MQTT client not initialized");
        return false;
    }

    _listening = true;
    mosquitto_loop_start(_mosq);

    int result = mosquitto_connect(_mosq, "localhost", _broker_port, 60);

    if (result != MOSQ_ERR_SUCCESS) {
        L_ERROR(
            "Failed to connect to MQTT broker: {}", mosquitto_strerror(result));
        mosquitto_loop_stop(_mosq, true);
        _listening = false;
        return false;
    }

    std::unique_lock<std::mutex> lock(_connect_mutex);
    if (!_connect_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
            [this]() { return _connected.load(); })) {
        L_ERROR("MQTT connection timed out");
        mosquitto_disconnect(_mosq);
        mosquitto_loop_stop(_mosq, true);
        _listening = false;
        return false;
    }

    return _connected.load();
}

void MqttListener::disconnect() noexcept
{
    if (_mosq && _connected) {
        mosquitto_disconnect(_mosq);
        _connected = false;
    }
}

bool MqttListener::subscribe(const std::string& topic, int qos) noexcept
{
    if (!_mosq || !_connected) {
        L_ERROR("Cannot subscribe: not connected to broker");
        return false;
    }

    int result = mosquitto_subscribe(_mosq, nullptr, topic.c_str(), qos);

    if (result != MOSQ_ERR_SUCCESS) {
        L_ERROR("Failed to subscribe to topic {}: {}", topic,
            mosquitto_strerror(result));
        return false;
    }

    _subscribed_topics.push_back(topic);
    return true;
}

bool MqttListener::is_connected() const noexcept { return _connected; }

void MqttListener::start_listening() noexcept
{
    if (_mosq && _connected && !_listening) {
        _listening = true;
        mosquitto_loop_start(_mosq);
    }
}

void MqttListener::stop_listening() noexcept
{
    _should_reconnect = false;
    if (_mosq && _listening) {
        mosquitto_loop_stop(_mosq, true);
        _listening = false;
    }
}

void MqttListener::on_connect(
    struct mosquitto* /*mosq*/, void* userdata, int rc)
{
    auto* listener = static_cast<MqttListener*>(userdata);

    if (rc == 0) {
        L_INFO("Connected to MQTT broker successfully");
        listener->_connected = true;
    } else {
        L_ERROR("MQTT connection failed with code: {}", rc);
        listener->_connected = false;
    }
    listener->_connect_cv.notify_all();
}

void MqttListener::on_disconnect(
    struct mosquitto* /*mosq*/, void* userdata, int rc)
{
    auto* listener       = static_cast<MqttListener*>(userdata);
    listener->_connected = false;

    if (rc != 0 && listener->_should_reconnect.load()) {
        L_ERROR(
            "Unexpected MQTT disconnection (code: {}), reconnecting...", rc);
        listener->try_reconnect();
    } else if (rc != 0) {
        L_ERROR("Unexpected MQTT disconnection (code: {})", rc);
    } else {
        L_INFO("Disconnected from MQTT broker");
    }
}

void MqttListener::on_message(struct mosquitto* /*mosq*/, void* userdata,
    const struct mosquitto_message* message)
{
    auto* listener = static_cast<MqttListener*>(userdata);

    if (!message || !message->payload) {
        return;
    }

    std::string topic(message->topic);
    std::string payload(
        static_cast<char*>(message->payload), message->payloadlen);

    listener->handle_message(topic, payload);
}

void MqttListener::on_subscribe(struct mosquitto* /*mosq*/, void* /*userdata*/,
    int mid, int qos_count, const int* granted_qos)
{
    L_INFO("Subscription confirmed (mid: {}, granted QoS: {})", mid,
        granted_qos[mid]);
}

void MqttListener::handle_message(
    const std::string& topic, const std::string& payload) noexcept
{
    for (const auto& [pattern, callback] : _handlers) {
        bool result = false;
        if (mosquitto_topic_matches_sub(pattern.c_str(), topic.c_str(), &result)
                == MOSQ_ERR_SUCCESS
            && result) {
            callback(topic, payload, _database, _ws_server);
            return;
        }
    }
}

void MqttListener::try_reconnect() noexcept
{
    if (_reconnecting.exchange(true)) {
        return;
    }

    _reconnect_thread = std::thread([this]() {
        auto delay = _reconnect_delay;
        while (_should_reconnect.load() && !_connected.load()) {
            std::this_thread::sleep_for(delay);
            if (!_should_reconnect.load()) {
                break;
            }
            L_INFO("Attempting MQTT reconnection...");
            if (connect(2000)) {
                for (const auto& topic : _subscribed_topics) {
                    subscribe(topic, 1);
                }
                start_listening();
                _reconnecting = false;
                return;
            }
            delay = std::min(delay * 2, _max_reconnect_delay);
        }
        _reconnecting = false;
    });
}

} // namespace f2z
