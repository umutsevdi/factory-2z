#include "database.h"
#include "tables.h"
#include "util.h"

#include <memory>
#include <sstream>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

#include <sqlite3.h>

namespace f2z {

namespace {

    db::SensorData sensor_data;
    db::Devices devices;
    db::SceneObjects scene_objects;

} // namespace

class SqliteDatabase : public Database {
public:
    SqliteDatabase() = default;
    ~SqliteDatabase() override { disconnect(); }

    bool connect(const AppConfig& config) override
    {
        try {
            sqlpp::sqlite3::connection_config sqlite_cfg;
            sqlite_cfg.path_to_database = config.database.path;
            sqlite_cfg.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
            _db = std::make_unique<sqlpp::sqlite3::connection>(sqlite_cfg);
            L_INFO("Connected to SQLite database: {}", config.database.path);
            create_table_if_not_exists();
            seed_scene_objects();
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
            (*_db)(sqlpp::insert_into(sensor_data)
                    .set(sensor_data.topic  = topic,
                        sensor_data.payload = payload));
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
            for (const auto& row :
                (*_db)(sqlpp::select(devices.mqtt_broker_host,
                    devices.mqtt_broker_port, devices.sensors)
                        .from(devices)
                        .where(devices.machine_id == machine_id
                            and devices.token == token))) {
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
                    return std::nullopt;
                }
                return config;
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            L_ERROR("Database get_device_config error: {}", e.what());
            return std::nullopt;
        }
    }

    std::optional<Json::Value> get_scene() override
    {
        try {
            Json::Value result(Json::arrayValue);

            auto build_object = [&](const auto& row) {
                Json::Value obj;
                obj["id"]            = std::string(row.id);
                obj["label"]         = std::string(row.label);
                obj["position"]["x"] = static_cast<double>(row.position_x);
                obj["position"]["y"] = static_cast<double>(row.position_y);
                obj["position"]["z"] = static_cast<double>(row.position_z);
                obj["size"]["x"]     = static_cast<double>(row.size_x);
                obj["size"]["y"]     = static_cast<double>(row.size_y);
                obj["size"]["z"]     = static_cast<double>(row.size_z);

                if (!row.rotation_x.is_null() && !row.rotation_y.is_null()
                    && !row.rotation_z.is_null()) {
                    obj["rotation"]["x"] = static_cast<double>(row.rotation_x);
                    obj["rotation"]["y"] = static_cast<double>(row.rotation_y);
                    obj["rotation"]["z"] = static_cast<double>(row.rotation_z);
                }

                obj["description"] = std::string(row.description);

                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(
                    builder.newCharReader());

                std::string metadata_str = std::string(row.metadata);
                Json::Value metadata_json;
                std::string errs;
                if (reader->parse(metadata_str.c_str(),
                        metadata_str.c_str() + metadata_str.size(),
                        &metadata_json, &errs)) {
                    obj["metadata"] = metadata_json;
                } else {
                    obj["metadata"] = Json::objectValue;
                }

                std::string connections_str = std::string(row.connections);
                Json::Value connections_json;
                if (reader->parse(connections_str.c_str(),
                        connections_str.c_str() + connections_str.size(),
                        &connections_json, &errs)) {
                    obj["connections"] = connections_json;
                } else {
                    obj["connections"] = Json::arrayValue;
                }

                std::string metrics_str = std::string(row.telemetry_metrics);
                Json::Value metrics_json;
                if (reader->parse(metrics_str.c_str(),
                        metrics_str.c_str() + metrics_str.size(), &metrics_json,
                        &errs)) {
                    obj["telemetryMetrics"] = metrics_json;
                } else {
                    obj["telemetryMetrics"] = Json::arrayValue;
                }

                result.append(obj);
            };

            for (const auto& row : (*_db)(sqlpp::select(scene_objects.id,
                     scene_objects.label, scene_objects.position_x,
                     scene_objects.position_y, scene_objects.position_z,
                     scene_objects.size_x, scene_objects.size_y,
                     scene_objects.size_z, scene_objects.rotation_x,
                     scene_objects.rotation_y, scene_objects.rotation_z,
                     scene_objects.description, scene_objects.metadata,
                     scene_objects.connections, scene_objects.telemetry_metrics)
                         .from(scene_objects)
                         .unconditionally())) {
                build_object(row);
            }

            Json::Value scene;
            scene["objects"] = result;
            return scene;
        } catch (const std::exception& e) {
            L_ERROR("Database get_scene error: {}", e.what());
            return std::nullopt;
        }
    }

    std::pair<std::string, std::string> resolve_mqtt_topic(
        const std::string& topic) override
    {
        auto parts = split_string(topic, '/');
        if (parts.size() < 3) {
            return { "", "" };
        }
        std::string device_id = parts[1];
        std::string metric    = parts[2];

        try {
            for (const auto& row : (*_db)(sqlpp::select(scene_objects.id)
                         .from(scene_objects)
                         .where(scene_objects.mqtt_device_id == device_id))) {
                return { std::string(row.id), metric };
            }
        } catch (const std::exception& e) {
            L_ERROR("Database resolve_mqtt_topic error: {}", e.what());
        }
        return { "", "" };
    }

    void disconnect() override { _db.reset(); }

    void reload() override { }

    bool save_sensor(uint64_t origin, std::string topic, double data) override
    {
        (void)origin;
        (void)topic;
        (void)data;
        return false;
    }

private:
    static std::vector<std::string> split_string(
        const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream stream(s);
        while (std::getline(stream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void create_table_if_not_exists()
    {
        try {
            _db->execute(R"(
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
            _db->execute(R"(
                CREATE TABLE IF NOT EXISTS sensor_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    topic TEXT NOT NULL,
                    payload TEXT,
                    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            )");
            _db->execute(R"(
                CREATE TABLE IF NOT EXISTS scene_objects (
                    id TEXT PRIMARY KEY,
                    label TEXT NOT NULL,
                    position_x REAL NOT NULL,
                    position_y REAL NOT NULL,
                    position_z REAL NOT NULL,
                    size_x REAL NOT NULL,
                    size_y REAL NOT NULL,
                    size_z REAL NOT NULL,
                    rotation_x REAL,
                    rotation_y REAL,
                    rotation_z REAL,
                    description TEXT NOT NULL,
                    metadata TEXT NOT NULL,
                    connections TEXT NOT NULL,
                    telemetry_metrics TEXT NOT NULL,
                    mqtt_device_id TEXT
                )
            )");
        } catch (const std::exception& e) {
            L_ERROR("Table creation error: {}", e.what());
        }
    }

    void seed_scene_objects()
    {
        struct SceneSeed {
            const char* id;
            const char* label;
            double px, py, pz;
            double sx, sy, sz;
            const char* rx;
            const char* ry;
            const char* rz;
            const char* description;
            const char* metadata;
            const char* connections;
            const char* telemetry_metrics;
            const char* mqtt_device_id;
        };

        static constexpr SceneSeed seeds[] = {
            { "obj-1", "Press A", 0, 0.5, 0, 1, 1, 1, nullptr, nullptr, nullptr,
                "Hydraulic stamping press, primary production line.",
                R"({"status":"operational","lastService":"2026-03-01","cycleCount":184220,"pressureBar":320})",
                R"([{"to_id":"obj-2"}])",
                R"([{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"}])",
                "press-a" },
            { "obj-2", "Conveyor 1", 3, 0.25, 0, 4, 0.5, 1, nullptr, nullptr,
                nullptr,
                "Main feed conveyor connecting Press A to the QC station.",
                R"({"status":"operational","speedMps":0.8,"lengthM":4})",
                R"([{"to_id":"obj-6"}])",
                R"([{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"}])",
                "conveyor-1" },
            { "obj-3", "Robotic Arm", -3, 1, 1, 1, 2, 1, nullptr, nullptr,
                nullptr,
                "6-axis pick-and-place arm for finished part handling.",
                R"({"status":"idle","lastJob":"2026-05-19T18:42:00Z","payloadKg":12})",
                R"([{"to_id":"obj-1"}])",
                R"([{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"}])",
                "robotic-arm" },
            { "obj-4", "Storage Rack", -2, 1.5, -3, 2, 3, 1.5, nullptr, nullptr,
                nullptr, "Buffer rack for in-progress assemblies.",
                R"({"status":"operational","capacity":24,"occupied":17})",
                R"([{"to_id":"obj-3"}])", "[]", nullptr },
            { "obj-5", "Control Cabinet", 4, 1, -3, 1.2, 2, 0.8, nullptr,
                nullptr, nullptr, "PLC and power distribution for cell 2.",
                R"({"status":"operational","temperatureC":38,"voltageV":400})",
                R"([{"to_id":"obj-1"},{"to_id":"obj-2"}])",
                R"([{"name":"temperature","unit":"\u00b0C"},{"name":"voltage","unit":"V"}])",
                "control-cabinet" },
            { "obj-6", "Pallet", 1, 0.15, 3, 1.2, 0.3, 1.2, nullptr, nullptr,
                nullptr, "EUR-pallet staging slot.",
                R"({"status":"loaded","itemCount":6})", "[]", "[]", nullptr },
        };

        for (const auto& s : seeds) {
            try {
                std::string sql = R"(
                    INSERT OR IGNORE INTO scene_objects (
                        id, label, position_x, position_y, position_z,
                        size_x, size_y, size_z,
                        rotation_x, rotation_y, rotation_z,
                        description, metadata, connections, telemetry_metrics,
                        mqtt_device_id
                    ) VALUES (')";
                sql += s.id;
                sql += "','";
                sql += s.label;
                sql += "',";
                sql += std::to_string(s.px);
                sql += ",";
                sql += std::to_string(s.py);
                sql += ",";
                sql += std::to_string(s.pz);
                sql += ",";
                sql += std::to_string(s.sx);
                sql += ",";
                sql += std::to_string(s.sy);
                sql += ",";
                sql += std::to_string(s.sz);
                sql += ",";
                if (s.rx)
                    sql += s.rx;
                else
                    sql += "NULL";
                sql += ",";
                if (s.ry)
                    sql += s.ry;
                else
                    sql += "NULL";
                sql += ",";
                if (s.rz)
                    sql += s.rz;
                else
                    sql += "NULL";
                sql += ",'";
                sql += s.description;
                sql += "','";
                sql += s.metadata;
                sql += "','";
                sql += s.connections;
                sql += "','";
                sql += s.telemetry_metrics;
                sql += "',";
                if (s.mqtt_device_id) {
                    sql += "'";
                    sql += s.mqtt_device_id;
                    sql += "'";
                } else {
                    sql += "NULL";
                }
                sql += ")";
                _db->execute(sql);
            } catch (const std::exception& e) {
                L_ERROR("Scene seed error for {}: {}", s.id, e.what());
            }
        }
        L_INFO("Seeded {} scene objects", sizeof(seeds) / sizeof(seeds[0]));
    }

    std::unique_ptr<sqlpp::sqlite3::connection> _db;
};

std::shared_ptr<Database> Database::create(const AppConfig& config)
{
    return std::make_shared<SqliteDatabase>();
}

} // namespace f2z
