#include "mqtt_listener.h"

#include <filesystem>
#include <format>
#include <print>
#include <thread>

namespace f2z {
namespace fs = std::filesystem;
static inline bool is_port_used(int port)
{
    return std::system(std::format("ss -tln | grep -q \":{}\"", port).c_str())
        == 0;
}

static inline bool is_ps_used()
{
    return std::system("pgrep -x mosquitto > /dev/null 2>&1") == 0;
}

bool start_broker(int port)
{
    fs::path executable = fs::current_path() / "mosquitto";
    if (!fs::exists(executable)) {
        L_ERROR(
            "Mosquitto broker executable not found next to application binary");
        return false;
    }
    std::println("Starting MQTT broker on port {}...", port);
    bool started = std::system(std::format(
                       "\"{}\" -d -p {} 2>/dev/null", executable.string(), port)
                           .c_str())
        == 0;
    if (started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        started = is_port_used(port);
    }

    if (started) {
        L_INFO("MQTT broker started successfully on port {}", port);
    } else {
        L_ERROR("Failed to start MQTT broker");
    }

    return started;
}

bool verify_broker_process(int port)
{
    if (is_port_used(port) && is_ps_used()) {
        L_INFO("MQTT broker detected at localhost:{}", port);
        return true;
    }
    return start_broker(port);
}

} // namespace f2z
