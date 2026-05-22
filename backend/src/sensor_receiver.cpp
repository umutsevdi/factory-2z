#include "database.h"
#include "websocket_server.h"
#include <memory>
namespace f2z {

static void handle_sensor(const std::string& topic, const std::string& payload,
    std::shared_ptr<f2z::Database> db, std::shared_ptr<f2z::WebSocketServer> ws)
{
    if (db) {
        db->insert_sensor_data(topic, payload);
    }
    if (ws && ws->is_running()) {
        ws->broadcast_json("sensor_update", topic, payload);
    }
}

} // namespace f2z
