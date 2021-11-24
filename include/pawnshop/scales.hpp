#pragma once

#include <chrono>
#include <fstream>
#include <future>
#include <optional>
#include <string>
#include <toml++/toml_table.hpp>

namespace pawnshop {

struct ScalesConfig {
    std::string uart_path;

    ScalesConfig(const toml::table& table);
};

class Scales {
    Scales(const std::string serialPath);
public:
    Scales(const std::unique_ptr<ScalesConfig> conf);
    ~Scales();
    std::optional<double> getWeight();
    bool poweredOn(
        std::chrono::duration<int> timeout = std::chrono::seconds(10));

private:
    std::string serial_path;
    struct State {
        std::string unit;
        double weight;
        bool stable;
        bool container;
    };
    std::optional<State> parse(const std::string line);
};

}  // namespace pawnshop
