#include "pawnshop/rails.hpp"

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;
using namespace std::chrono_literals;
using namespace pawnshop::vec;

namespace pawnshop {

RailsConfig::RailsConfig(const toml::table &table) {
    gpio_chip = table["gpio_chip"].value<string>().value();

    x_axis = make_shared<AxisConfig>(*table["x_axis"].as_table());
    y_axis = make_shared<AxisConfig>(*table["y_axis"].as_table());
    z_axis = make_shared<AxisConfig>(*table["z_axis"].as_table());
}

void Rails::move(Vec3D newPos) {
    const Vec3D track = newPos - getPos();
    const Vec3D direction = normalize(track);
    std::array<std::thread, 3> movingAxes;
    for (size_t i = 0; i < movingAxes.size(); i++) {
        movingAxes[i] =
            std::thread(&Axis::move, &axes[i], newPos[i], direction[i]);
    }
    for (auto &t : movingAxes) {
        t.join();
    }
}

void Rails::calibrate() {
    for (auto &axis : axes) {
        axis.calibrate();
    }
}

Vec3D Rails::getPos() {
    Vec3D pos;
    auto i = pos.begin();
    for (auto &axis : axes) {
        *(i++) = axis.getPosition();
    }
    return pos;
}

}  // namespace pawnshop
