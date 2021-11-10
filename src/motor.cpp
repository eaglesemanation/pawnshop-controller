#include "pawnshop/motor.hpp"

#include <chrono>
#include <cmath>
#include <map>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace pawnshop {

Motor::Motor(gpiod::line clockLine, gpiod::line dirLine,
             bool inverted /* = false */
             )
    : inverted(inverted) {
    static const gpiod::line_request outputReq = {
        "pawnshop-motor", gpiod::line_request::DIRECTION_OUTPUT, 0};
    clockLine.request(outputReq);
    this->clockLine = clockLine;
    dirLine.request(outputReq);
    this->dirLine = dirLine;
    setDirection(POSITIVE);
}

Motor::Motor(const gpiod::chip gpioChip, const uint8_t clockLineOffset,
             const uint8_t dirLineOffset, bool inverted /* = false */
             )
    : Motor(gpioChip.get_line(clockLineOffset),
            gpioChip.get_line(dirLineOffset), inverted) {}

Motor::Motor(Motor&& src)
    : clockLine(std::move(src.clockLine)),
      dirLine(std::move(src.dirLine)),
      dir(src.dir),
      inverted(src.inverted),
      stopped(src.stopped),
      period(src.period.load()) {}

int16_t Motor::step() const {
    if (stopped) {
        return 0;
    }
    auto period = this->period.load();
    clockLine.set_value(1);
    std::this_thread::sleep_for(period / 2);
    clockLine.set_value(0);
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
    dirLine.set_value(((dir == POSITIVE) ^ inverted) ? 1 : 0);
    this->dir = dir;
}

Motor::Direction Motor::getDirection() const { return dir; }

}  // namespace pawnshop
