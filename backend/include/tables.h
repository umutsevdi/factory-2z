#pragma once

#include <sqlpp11/char_sequence.h>
#include <sqlpp11/data_types.h>
#include <sqlpp11/table.h>

namespace f2z::db {

namespace SensorData_ {
    struct Id {
        struct _alias_t {
            static constexpr const char _literal[] = "id";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T id;
                T& operator()() { return id; }
                const T& operator()() const { return id; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::bigint,
            sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
    };
    struct Topic {
        struct _alias_t {
            static constexpr const char _literal[] = "topic";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T topic;
                T& operator()() { return topic; }
                const T& operator()() const { return topic; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct Payload {
        struct _alias_t {
            static constexpr const char _literal[] = "payload";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T payload;
                T& operator()() { return payload; }
                const T& operator()() const { return payload; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::can_be_null>;
    };
    struct ReceivedAt {
        struct _alias_t {
            static constexpr const char _literal[] = "received_at";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T received_at;
                T& operator()() { return received_at; }
                const T& operator()() const { return received_at; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
    };
} // namespace SensorData_

struct SensorData
    : sqlpp::table_t<SensorData, SensorData_::Id, SensorData_::Topic,
          SensorData_::Payload, SensorData_::ReceivedAt> {
    struct _alias_t {
        static constexpr const char _literal[] = "sensor_data";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template <typename T> struct _member_t {
            T sensor_data;
            T& operator()() { return sensor_data; }
            const T& operator()() const { return sensor_data; }
        };
    };
};

namespace Devices_ {
    struct Id {
        struct _alias_t {
            static constexpr const char _literal[] = "id";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T id;
                T& operator()() { return id; }
                const T& operator()() const { return id; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::bigint,
            sqlpp::tag::must_not_insert, sqlpp::tag::must_not_update>;
    };
    struct MachineId {
        struct _alias_t {
            static constexpr const char _literal[] = "machine_id";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T machine_id;
                T& operator()() { return machine_id; }
                const T& operator()() const { return machine_id; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct Token {
        struct _alias_t {
            static constexpr const char _literal[] = "token";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T token;
                T& operator()() { return token; }
                const T& operator()() const { return token; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct MqttBrokerHost {
        struct _alias_t {
            static constexpr const char _literal[] = "mqtt_broker_host";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T mqtt_broker_host;
                T& operator()() { return mqtt_broker_host; }
                const T& operator()() const { return mqtt_broker_host; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::can_be_null>;
    };
    struct MqttBrokerPort {
        struct _alias_t {
            static constexpr const char _literal[] = "mqtt_broker_port";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T mqtt_broker_port;
                T& operator()() { return mqtt_broker_port; }
                const T& operator()() const { return mqtt_broker_port; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::integer, sqlpp::tag::can_be_null>;
    };
    struct Sensors {
        struct _alias_t {
            static constexpr const char _literal[] = "sensors";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T sensors;
                T& operator()() { return sensors; }
                const T& operator()() const { return sensors; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct CreatedAt {
        struct _alias_t {
            static constexpr const char _literal[] = "created_at";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T created_at;
                T& operator()() { return created_at; }
                const T& operator()() const { return created_at; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::time_point, sqlpp::tag::can_be_null>;
    };
} // namespace Devices_

struct Devices
    : sqlpp::table_t<Devices, Devices_::Id, Devices_::MachineId,
          Devices_::Token, Devices_::MqttBrokerHost, Devices_::MqttBrokerPort,
          Devices_::Sensors, Devices_::CreatedAt> {
    struct _alias_t {
        static constexpr const char _literal[] = "devices";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template <typename T> struct _member_t {
            T devices;
            T& operator()() { return devices; }
            const T& operator()() const { return devices; }
        };
    };
};

namespace SceneObjects_ {
    struct Id {
        struct _alias_t {
            static constexpr const char _literal[] = "id";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T id;
                T& operator()() { return id; }
                const T& operator()() const { return id; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct Label {
        struct _alias_t {
            static constexpr const char _literal[] = "label";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T label;
                T& operator()() { return label; }
                const T& operator()() const { return label; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct PositionX {
        struct _alias_t {
            static constexpr const char _literal[] = "position_x";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T position_x;
                T& operator()() { return position_x; }
                const T& operator()() const { return position_x; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct PositionY {
        struct _alias_t {
            static constexpr const char _literal[] = "position_y";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T position_y;
                T& operator()() { return position_y; }
                const T& operator()() const { return position_y; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct PositionZ {
        struct _alias_t {
            static constexpr const char _literal[] = "position_z";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T position_z;
                T& operator()() { return position_z; }
                const T& operator()() const { return position_z; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct SizeX {
        struct _alias_t {
            static constexpr const char _literal[] = "size_x";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T size_x;
                T& operator()() { return size_x; }
                const T& operator()() const { return size_x; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct SizeY {
        struct _alias_t {
            static constexpr const char _literal[] = "size_y";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T size_y;
                T& operator()() { return size_y; }
                const T& operator()() const { return size_y; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct SizeZ {
        struct _alias_t {
            static constexpr const char _literal[] = "size_z";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T size_z;
                T& operator()() { return size_z; }
                const T& operator()() const { return size_z; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::require_insert>;
    };
    struct RotationX {
        struct _alias_t {
            static constexpr const char _literal[] = "rotation_x";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T rotation_x;
                T& operator()() { return rotation_x; }
                const T& operator()() const { return rotation_x; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::can_be_null>;
    };
    struct RotationY {
        struct _alias_t {
            static constexpr const char _literal[] = "rotation_y";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T rotation_y;
                T& operator()() { return rotation_y; }
                const T& operator()() const { return rotation_y; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::can_be_null>;
    };
    struct RotationZ {
        struct _alias_t {
            static constexpr const char _literal[] = "rotation_z";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T rotation_z;
                T& operator()() { return rotation_z; }
                const T& operator()() const { return rotation_z; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::floating_point,
            sqlpp::tag::can_be_null>;
    };
    struct Description {
        struct _alias_t {
            static constexpr const char _literal[] = "description";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T description;
                T& operator()() { return description; }
                const T& operator()() const { return description; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct Metadata {
        struct _alias_t {
            static constexpr const char _literal[] = "metadata";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T metadata;
                T& operator()() { return metadata; }
                const T& operator()() const { return metadata; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct Connections {
        struct _alias_t {
            static constexpr const char _literal[] = "connections";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T connections;
                T& operator()() { return connections; }
                const T& operator()() const { return connections; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct TelemetryMetrics {
        struct _alias_t {
            static constexpr const char _literal[] = "telemetry_metrics";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T telemetry_metrics;
                T& operator()() { return telemetry_metrics; }
                const T& operator()() const { return telemetry_metrics; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::require_insert>;
    };
    struct MqttDeviceId {
        struct _alias_t {
            static constexpr const char _literal[] = "mqtt_device_id";
            using _name_t
                = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T> struct _member_t {
                T mqtt_device_id;
                T& operator()() { return mqtt_device_id; }
                const T& operator()() const { return mqtt_device_id; }
            };
        };
        using _traits
            = sqlpp::make_traits<sqlpp::text, sqlpp::tag::can_be_null>;
    };
} // namespace SceneObjects_

struct SceneObjects
    : sqlpp::table_t<SceneObjects, SceneObjects_::Id, SceneObjects_::Label,
          SceneObjects_::PositionX, SceneObjects_::PositionY,
          SceneObjects_::PositionZ, SceneObjects_::SizeX, SceneObjects_::SizeY,
          SceneObjects_::SizeZ, SceneObjects_::RotationX,
          SceneObjects_::RotationY, SceneObjects_::RotationZ,
          SceneObjects_::Description, SceneObjects_::Metadata,
          SceneObjects_::Connections, SceneObjects_::TelemetryMetrics,
          SceneObjects_::MqttDeviceId> {
    struct _alias_t {
        static constexpr const char _literal[] = "scene_objects";
        using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
        template <typename T> struct _member_t {
            T scene_objects;
            T& operator()() { return scene_objects; }
            const T& operator()() const { return scene_objects; }
        };
    };
};

} // namespace f2z::db
