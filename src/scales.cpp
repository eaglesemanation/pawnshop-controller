#include "pawnshop/scales.hpp"
#include <regex>
#include <iostream>
#include <numeric>

namespace pawnshop {

Scales::Scales(const std::string serialPath) : serialPath(serialPath) {}

Scales::~Scales() {}

std::optional<Scales::State> Scales::parse(const std::string line)
{
    const std::regex entryFormat("(ST|US),(GS|NT)([\\- ][0-9\\. ]{7})([a-z ]{3})",
                      std::regex_constants::ECMAScript | std::regex_constants::optimize);
    std::smatch submatch;
    Scales::State state;
    if (!regex_match(line, submatch, entryFormat))
    {
        return {};
    }
    auto subIt = submatch.begin();
    state.stable = *(++subIt) == "ST";
    state.container = *(++subIt) != "GS";
    std::string weight = *(++subIt);
    std::string weightFormatted;
    copy_if(weight.begin(), weight.end(), back_inserter(weightFormatted), [](const char c){
        return !isspace(c);
    });
    state.weight = stod(weightFormatted);
    std::string unit = *(++subIt);
    unit.erase(unit.begin(), find_if(unit.begin(), unit.end(), [](const char c){
        return !isspace(c);
    }));
    unit.erase(find_if(unit.begin(), unit.end(), [](const char c){
        return isspace(c);
    }), unit.end());
    state.unit = unit;
    return state;
}

double Scales::getWeight()
{
    std::ifstream serial(serialPath);
    std::array<double, 20> measurements;
    auto measurementsIter = measurements.begin();
    while (true)
    {
        std::string line;
        std::getline(serial, line);
        std::optional<Scales::State> state = parse(line);
        if (state.has_value())
        {
            if (state->stable)
            {
                *measurementsIter++ =  state->weight;
            }
        }
        if(measurementsIter == measurements.end()) {
            std::sort(measurements.begin(), measurements.end());
            const size_t offset = measurements.size() / 4;
            return std::accumulate(
                    measurements.begin() + offset,
                    measurements.end() - offset,
                    0.0
                ) / (offset * 2);
        }
    }
}

}