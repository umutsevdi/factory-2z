#pragma once

#include "util.h"
#include <json/json.h>
#include <memory>
#include <optional>
#include <string>

namespace f2z {

class Database {
public:
    static std::shared_ptr<Database> create(const AppConfig& config);

    Database()                           = default;
    Database(Database&&)                 = default;
    Database(const Database&)            = delete;
    Database& operator=(Database&&)      = default;
    Database& operator=(const Database&) = delete;
    virtual ~Database()                  = default;

    virtual bool connect(const AppConfig& config) = 0;
    virtual bool insert_sensor_data(
        const std::string& topic, const std::string& payload)
        = 0;
    virtual std::optional<Json::Value> get_device_config(
        const std::string& machine_id, const std::string& token)
        = 0;
    virtual void disconnect() = 0;
};

} // namespace f2z
