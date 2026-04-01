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

} // namespace f2z::db
