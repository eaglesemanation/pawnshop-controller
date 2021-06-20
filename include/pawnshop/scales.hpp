#pragma once
#include <fstream>
#include <string>
#include <future>
#include <optional>

namespace pawnshop {

class Scales {
public:
    Scales(const std::string serialPath);
    virtual ~Scales();
    double getWeight();
private:
    std::string serialPath;
    struct State {
        std::string unit;
        double weight;
        bool stable;
        bool container;
    };
    std::optional<State> parse(const std::string line);
};

}