#include "pawnshop/scales.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <regex>

#include "pawnshop/util.hpp"

namespace pawnshop {

Scales::Scales(const std::string serial_path) : serial_path(serial_path) {}

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
    std::ifstream serial(serial_path);
    std::array<double, 20> measurements;
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
            }
        }
        if (measurements_iter == measurements.end()) {
            std::sort(measurements.begin(), measurements.end());
            const size_t offset = measurements.size() / 4;
            return std::accumulate(measurements.begin() + offset,
                                   measurements.end() - offset, 0.0) /
                   (offset * 2);
        }
    }
}

bool Scales::poweredOn(std::chrono::duration<int> timeout) {
    std::ifstream serial(serial_path);
    auto line = getline_timeout(serial, timeout);
    return line.has_value();
}

}  // namespace pawnshop
