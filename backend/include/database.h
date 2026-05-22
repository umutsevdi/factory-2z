#pragma once

#include "util.h"
#include <json/json.h>
#include <memory>
#include <optional>
#include <string>

namespace f2z {

class DeviceProfile {
    uint64_t id;
    std::string machine_id;
    std::string token;
    std::string host;
    int port;
    std::vector<std::string> sensors;
    std::string created_at;
};

class SensorData {
    uint64_t id;
    std::string topic;
    std::string payload;
    std::string received_at;
};

class Database {
public:
    static std::shared_ptr<Database> create(const AppConfig& config);

    Database()                           = default;
    Database(Database&&)                 = default;
    Database(const Database&)            = delete;
    Database& operator=(Database&&)      = default;
    Database& operator=(const Database&) = delete;
    virtual ~Database()                  = default;

    /* Get a device by id */
    std::weak_ptr<DeviceProfile> get_profile_by_id(uint64_t);
    /* Reload all device profiles back to the memory */
    virtual void reload() = 0;
    virtual bool save_sensor(uint64_t origin, std::string topic, double data)
        = 0;

    virtual bool connect(const AppConfig& config) = 0;
    virtual bool insert_sensor_data(
        const std::string& topic, const std::string& payload) = 0;
    virtual std::optional<Json::Value> get_device_config(
        const std::string& machine_id, const std::string& token) = 0;
    virtual std::optional<Json::Value> get_scene()               = 0;
    virtual std::pair<std::string, std::string> resolve_mqtt_topic(
        const std::string& topic) = 0;
    virtual void disconnect()     = 0;
};

} // namespace f2z
