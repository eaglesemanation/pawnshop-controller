#include "pawnshop/motor.hpp"

#include <chrono>
#include <cmath>
#include <map>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono_literals;

namespace pawnshop {

MotorConfig::MotorConfig(const toml::table& table) {
    clock_pin = table["clock_pin"].value<size_t>().value();
    direction_pin = table["direction_pin"].value<size_t>().value();
    counter_clockwire = table["counter_clockwise"].value<bool>().value();
}

Motor::Motor(gpiod::line clock_line, gpiod::line dir_line,
             bool inverted /* = false */
             )
    : inverted(inverted) {
    static const gpiod::line_request output_req = {
        "pawnshop-motor", gpiod::line_request::DIRECTION_OUTPUT, 0};
    clock_line.request(output_req);
    this->clock_line = clock_line;
    dir_line.request(output_req);
    this->dir_line = dir_line;
    setDirection(POSITIVE);
}

Motor::Motor(const gpiod::chip gpio_chip, const uint8_t clock_line_offset,
             const uint8_t dir_line_offset, bool inverted /* = false */
             )
    : Motor(gpio_chip.get_line(clock_line_offset),
            gpio_chip.get_line(dir_line_offset), inverted) {}

Motor::Motor(Motor&& src)
    : clock_line(std::move(src.clock_line)),
      dir_line(std::move(src.dir_line)),
      dir(src.dir),
      inverted(src.inverted),
      stopped(src.stopped),
      period(src.period.load()) {}

int16_t Motor::step() const {
    if (stopped) {
        return 0;
    }
    auto period = this->period.load();
    clock_line.set_value(1);
    std::this_thread::sleep_for(period / 2);
    clock_line.set_value(0);
    std::this_thread::sleep_for(period / 2);
    return static_cast<int16_t>(dir);
}

void Motor::setFrequency(const uint32_t hz) {
    if (hz == 0) {
        stopped = true;
        return;
    }
    stopped = false;
    auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(1s) / hz;
    setPeriod(period);
}

int32_t Motor::getFrequency() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(1s) /
           period.load();
}

void Motor::setPeriod(const std::chrono::nanoseconds period) {
    this->period = period;
}

void Motor::setDirection(Motor::Direction dir) {
    dir_line.set_value(((dir == POSITIVE) ^ inverted) ? 1 : 0);
    this->dir = dir;
}

Motor::Direction Motor::getDirection() const { return dir; }

}  // namespace pawnshop
