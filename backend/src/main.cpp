#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace f2z {

class DataProcessor {
public:
    explicit DataProcessor(const std::string& cfg_path) : _cfg_path(cfg_path) {}

    void process_sensor(const std::string& data) {
        std::cout << "Processing sensor data: " << data << std::endl;
        _pending_data.push_back(data);
    }

    [[nodiscard]] std::size_t pending_count() const noexcept {
        return _pending_data.size();
    }

private:
    std::vector<std::string> _pending_data;
    std::string _cfg_path;
};

} // namespace f2z

int main() {
    auto processor = std::make_unique<f2z::DataProcessor>("config.json");
    
    processor->process_sensor("temperature:72.5");
    processor->process_sensor("vibration:0.023");
    
    std::cout << "Pending items: " << processor->pending_count() << std::endl;
    
    return 0;
}
