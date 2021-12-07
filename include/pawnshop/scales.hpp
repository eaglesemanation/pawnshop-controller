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
    size_t sample_size;

    ScalesConfig(const toml::table& table);
};

class Scales {
public:
    Scales(std::unique_ptr<const ScalesConfig> conf);
    ~Scales();
    /**
     * Measures for n consecutive times with all measurements being stable,
     * where n is defined in configuration file as "sample_size". In case
     * unstable measurement met - starts over.
     *
     * @returns Either median of all measurements, or {} in case scales are
     * turned off
     */
    std::optional<double> getWeight();
    bool poweredOn(
        std::chrono::duration<int> timeout = std::chrono::seconds(10));

private:
    std::unique_ptr<const ScalesConfig> conf;
    struct State {
        std::string unit;
        double weight;
        bool stable;
        bool container;
    };
    std::optional<State> parse(const std::string line);
};

}  // namespace pawnshop
