#include "database.h"
#include "tables.h"
#include "util.h"

#include <memory>

#include <sqlpp11/postgresql/postgresql.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

#include <sqlite3.h>

namespace f2z {

namespace {

    db::SensorData sensor_data;
    db::Devices devices;

} // namespace

class SqlppDatabase : public Database {
public:
    SqlppDatabase() = default;
    ~SqlppDatabase() override { disconnect(); }

    bool connect(const AppConfig& config) override
    {
        try {
            if (config.database.type == "sqlite") {
                sqlpp::sqlite3::connection_config sqlite_cfg;
                sqlite_cfg.path_to_database = config.database.connection_string;
                sqlite_cfg.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
                _sqlite3_db
                    = std::make_unique<sqlpp::sqlite3::connection>(sqlite_cfg);
                _db_type = DbType::SQLite;
            } else if (config.database.type == "postgresql") {
                sqlpp::postgresql::connection_config pg_cfg;
                pg_cfg.dbname = config.database.connection_string;
                _postgresql_db
                    = std::make_unique<sqlpp::postgresql::connection>(pg_cfg);
                _db_type = DbType::PostgreSQL;
            } else {
                L_ERROR("Unsupported database type: {}", config.database.type);
                return false;
            }
            L_INFO("Connected to {} database", config.database.type);
            create_table_if_not_exists();
            return true;
        } catch (const std::exception& e) {
            L_ERROR("Database connection error: {}", e.what());
            return false;
        }
    }

    bool insert_sensor_data(
        const std::string& topic, const std::string& payload) override
    {
        try {
            if (_db_type == DbType::SQLite) {
                (*_sqlite3_db)(sqlpp::insert_into(sensor_data)
                        .set(sensor_data.topic  = topic,
                            sensor_data.payload = payload));
            } else if (_db_type == DbType::PostgreSQL) {
                (*_postgresql_db)(sqlpp::insert_into(sensor_data)
                        .set(sensor_data.topic  = topic,
                            sensor_data.payload = payload));
            }
            return true;
        } catch (const std::exception& e) {
            L_ERROR("Database insert error: {}", e.what());
            return false;
        }
    }

    std::optional<Json::Value> get_device_config(
        const std::string& machine_id, const std::string& token) override
    {
        try {
            auto make_config = [&](const auto& row) -> Json::Value {
                Json::Value config;
                config["machine_id"]       = machine_id;
                config["mqtt_broker_host"] = row.mqtt_broker_host.is_null()
                    ? "localhost"
                    : std::string(row.mqtt_broker_host);
                config["mqtt_broker_port"] = row.mqtt_broker_port.is_null()
                    ? 1883
                    : static_cast<int>(row.mqtt_broker_port);
                std::string sensors_str
                    = row.sensors.is_null() ? "" : std::string(row.sensors);
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(
                    builder.newCharReader());
                Json::Value sensors_json;
                std::string errs;
                if (reader->parse(sensors_str.c_str(),
                        sensors_str.c_str() + sensors_str.size(), &sensors_json,
                        &errs)) {
                    config["sensors"] = sensors_json;
                } else {
                    L_ERROR("Failed to parse sensors JSON: {}", errs);
                    return Json::Value();
                }
                return config;
            };

            if (_db_type == DbType::SQLite) {
                for (const auto& row :
                    (*_sqlite3_db)(sqlpp::select(devices.mqtt_broker_host,
                        devices.mqtt_broker_port, devices.sensors)
                            .from(devices)
                            .where(devices.machine_id == machine_id
                                and devices.token == token))) {
                    auto config = make_config(row);
                    if (!config.empty()) {
                        return config;
                    }
                    return std::nullopt;
                }
            } else if (_db_type == DbType::PostgreSQL) {
                for (const auto& row :
                    (*_postgresql_db)(sqlpp::select(devices.mqtt_broker_host,
                        devices.mqtt_broker_port, devices.sensors)
                            .from(devices)
                            .where(devices.machine_id == machine_id
                                and devices.token == token))) {
                    auto config = make_config(row);
                    if (!config.empty()) {
                        return config;
                    }
                    return std::nullopt;
                }
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            L_ERROR("Database get_device_config error: {}", e.what());
            return std::nullopt;
        }
    }

    void disconnect() override
    {
        _sqlite3_db.reset();
        _postgresql_db.reset();
        _db_type = DbType::None;
    }

private:
    void create_table_if_not_exists()
    {
        try {
            if (_db_type == DbType::SQLite) {
                _sqlite3_db->execute(R"(
                    CREATE TABLE IF NOT EXISTS sensor_data (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        topic TEXT NOT NULL,
                        payload TEXT,
                        received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                    )
                )");
                _sqlite3_db->execute(R"(
                    CREATE TABLE IF NOT EXISTS devices (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        machine_id TEXT UNIQUE NOT NULL,
                        token TEXT UNIQUE NOT NULL,
                        mqtt_broker_host TEXT DEFAULT 'localhost',
                        mqtt_broker_port INTEGER DEFAULT 1883,
                        sensors TEXT NOT NULL,
                        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                    )
                )");
            } else if (_db_type == DbType::PostgreSQL) {
                _postgresql_db->execute(R"(
                    CREATE TABLE IF NOT EXISTS sensor_data (
                        id SERIAL PRIMARY KEY,
                        topic TEXT NOT NULL,
                        payload TEXT,
                        received_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
                    )
                )");
                _postgresql_db->execute(R"(
                    CREATE TABLE IF NOT EXISTS devices (
                        id SERIAL PRIMARY KEY,
                        machine_id TEXT UNIQUE NOT NULL,
                        token TEXT UNIQUE NOT NULL,
                        mqtt_broker_host TEXT DEFAULT 'localhost',
                        mqtt_broker_port INTEGER DEFAULT 1883,
                        sensors TEXT NOT NULL,
                        created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
                    )
                )");
            }
        } catch (const std::exception& e) {
            L_ERROR("Table creation error: {}", e.what());
        }
    }

    enum class DbType { None, SQLite, PostgreSQL };
    DbType _db_type = DbType::None;
    std::unique_ptr<sqlpp::sqlite3::connection> _sqlite3_db;
    std::unique_ptr<sqlpp::postgresql::connection> _postgresql_db;
};

std::shared_ptr<Database> Database::create(const AppConfig& config)
{
    if (config.database.type != "sqlite"
        && config.database.type != "postgresql") {
        L_ERROR("Unsupported database type: {}", config.database.type);
        return nullptr;
    }
    return std::make_shared<SqlppDatabase>();
}

} // namespace f2z
