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

    axes[0] = make_unique<AxisConfig>(*table["x_axis"].as_table());
    axes[1] = make_unique<AxisConfig>(*table["y_axis"].as_table());
    axes[2] = make_unique<AxisConfig>(*table["z_axis"].as_table());
}

Rails::Rails(const std::unique_ptr<RailsConfig> conf) : chip{conf->gpio_chip} {
    for (size_t i = 0; i < axes.size(); i++) {
        axes[i] = make_unique<Axis>(chip, std::move(conf->axes[i]));
    }
}

void Rails::move(Vec3D newPos) {
    const Vec3D track = newPos - getPos();
    const Vec3D direction = normalize(track);
    std::array<std::thread, 3> movingAxes;
    for (size_t i = 0; i < movingAxes.size(); i++) {
        movingAxes[i] =
            std::thread(&Axis::move, axes[i].get(), newPos[i], direction[i]);
    }
    for (auto &t : movingAxes) {
        t.join();
    }
}

void Rails::calibrate() {
    for (auto &axis : axes) {
        axis->calibrate();
    }
}

Vec3D Rails::getPos() {
    Vec3D pos;
    auto i = pos.begin();
    for (auto &axis : axes) {
        *(i++) = axis->getPosition();
    }
    return pos;
}

}  // namespace pawnshop
