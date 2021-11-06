#include "pawnshop/scales.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <regex>

#include "pawnshop/util.hpp"

namespace pawnshop {

Scales::Scales(const std::string serialPath) : serialPath(serialPath) {}

Scales::~Scales() {}

std::optional<Scales::State> Scales::parse(const std::string line) {
    const std::regex entryFormat(
        "(ST|US),(GS|NT)([\\- ][0-9\\. ]{7})([a-z ]{3})",
        std::regex_constants::ECMAScript | std::regex_constants::optimize);
    std::smatch submatch;
    Scales::State state;
    if (!regex_match(line, submatch, entryFormat)) {
        return {};
    }
    auto subIt = submatch.begin();
    state.stable = *(++subIt) == "ST";
    state.container = *(++subIt) != "GS";
    std::string weight = *(++subIt);
    std::string weightFormatted;
    copy_if(weight.begin(), weight.end(), back_inserter(weightFormatted),
            [](const char c) { return !isspace(c); });
    state.weight = stod(weightFormatted);
    std::string unit = *(++subIt);
    unit.erase(unit.begin(), find_if(unit.begin(), unit.end(),
                                     [](const char c) { return !isspace(c); }));
    unit.erase(find_if(unit.begin(), unit.end(),
                       [](const char c) { return isspace(c); }),
               unit.end());
    state.unit = unit;
    return state;
}

std::optional<double> Scales::getWeight() {
    std::ifstream serial(serialPath);
    std::array<double, 20> measurements;
    auto measurementsIter = measurements.begin();
    while (true) {
        std::chrono::seconds timeout(10);
        auto line = getline_timeout(serial, timeout);
        if (!line) {
            return {};
        }
        std::optional<Scales::State> state = parse(line.value());
        if (State) {
            if (state->stable) {
                *measurementsIter++ = state->weight;
            }
        }
        if (measurementsIter == measurements.end()) {
            std::sort(measurements.begin(), measurements.end());
            const size_t offset = measurements.size() / 4;
            return std::accumulate(measurements.begin() + offset,
                                   measurements.end() - offset, 0.0) /
                   (offset * 2);
        }
    }
}

bool Scales::poweredOn(std::chrono::duration<int> timeout) {
    std::ifstream serial(serialPath);
    auto line = getline_timeout(serial, timeout);
    return line.has_value();
}

}  // namespace pawnshop
