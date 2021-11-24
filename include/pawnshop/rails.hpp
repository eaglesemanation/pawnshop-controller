#pragma once

#include <array>
#include <mutex>
#include <shared_mutex>
#include <gpiod.hpp>

#include "axis.hpp"
#include "vec.hpp"

namespace pawnshop {

struct RailsConfig {
    std::string gpio_chip;
    std::array<std::unique_ptr<AxisConfig>, 3> axes;

    RailsConfig(const toml::table& table);
};

class Rails {
public:
    Rails(const std::unique_ptr<RailsConfig> conf);
    Rails(const Rails&) = delete;
    Rails(Rails&&) = default;
    ~Rails() = default;
    void move(const pawnshop::vec::Vec3D newPos);
    void calibrate();
    pawnshop::vec::Vec3D getPos();

private:
    std::array<std::unique_ptr<Axis>, 3> axes;
    gpiod::chip chip;
};

}  // namespace pawnshop
