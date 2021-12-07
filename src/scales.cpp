#include "pawnshop/scales.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <regex>

#include "pawnshop/util.hpp"

using namespace std;

namespace pawnshop {

ScalesConfig::ScalesConfig(const toml::table& table) {
    uart_path = table["uart_path"].value<string>().value();
    sample_size = table["sample_size"].value<size_t>().value_or(20);
}

Scales::Scales(unique_ptr<const ScalesConfig> conf) : conf{move(conf)} {}

Scales::~Scales() {}

std::optional<Scales::State> Scales::parse(const std::string line) {
    const std::regex entry_format(
        "(ST|US),(GS|NT)([\\- ][0-9\\. ]{7})([a-z ]{3})",
        std::regex_constants::ECMAScript | std::regex_constants::optimize);
    std::smatch submatch;
    Scales::State state;
    if (!regex_match(line, submatch, entry_format)) {
        return {};
    }
    auto sub_it = submatch.begin();
    state.stable = *(++sub_it) == "ST";
    state.container = *(++sub_it) != "GS";
    std::string weight = *(++sub_it);
    std::string weight_formatted;
    copy_if(weight.begin(), weight.end(), back_inserter(weight_formatted),
            [](const char c) { return !isspace(c); });
    state.weight = stod(weight_formatted);
    std::string unit = *(++sub_it);
    unit.erase(unit.begin(), find_if(unit.begin(), unit.end(),
                                     [](const char c) { return !isspace(c); }));
    unit.erase(find_if(unit.begin(), unit.end(),
                       [](const char c) { return isspace(c); }),
               unit.end());
    state.unit = unit;
    return state;
}

std::optional<double> Scales::getWeight() {
    std::ifstream serial(conf->uart_path);
    std::vector<double> measurements;
    measurements.resize(conf->sample_size);
    auto measurements_iter = measurements.begin();
    while (true) {
        std::chrono::seconds timeout(10);
        auto line = getline_timeout(serial, timeout);
        if (!line) {
            return {};
        }
        std::optional<Scales::State> state = parse(line.value());
        if (state) {
            if (state->stable) {
                *measurements_iter++ = state->weight;
            } else {
                measurements_iter = measurements.begin();
            }
        }
        if (measurements_iter == measurements.end()) {
            std::sort(measurements.begin(), measurements.end());
            return measurements[measurements.size() / 2];
        }
    }
}

bool Scales::poweredOn(std::chrono::duration<int> timeout) {
    std::ifstream serial(conf->uart_path);
    auto line = getline_timeout(serial, timeout);
    return line.has_value();
}

}  // namespace pawnshop
