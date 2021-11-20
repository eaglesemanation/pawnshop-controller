#include "pawnshop/axis.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

namespace pawnshop {

Axis::Axis(const double axis_length, const uint32_t step_count,
           const double min_speed, const double max_speed,
           const double acceleration, Motor &&motor, LimitSwitch &&negative)
    : motor(std::move(motor)),
      negative(std::move(negative)),
      axis_length(axis_length),
      step_length(axis_length / step_count),
      MIN_SPEED(min_speed),
      MAX_SPEED(max_speed),
      ACCELERATION(acceleration) {}

Axis::Axis(Axis &&src)
    : motor(std::move(src.motor)),
      negative(std::move(src.negative)),
      axis_length(src.axis_length),
      step_length(src.step_length),
      MIN_SPEED(src.MIN_SPEED),
      MAX_SPEED(src.MAX_SPEED),
      ACCELERATION(src.ACCELERATION) {}

void Axis::calibrate() {
    // TODO: Calibration for case with 2 limit switches
    setSpeed(-MIN_SPEED);
    while (negative.has_value() && !negative.value()) motor.step();
    setPosition(0.0);
}

void Axis::move(const double new_pos, const double scaling) {
    const double old_pos = getPosition();
    const double S = std::abs(new_pos - old_pos);
    const int8_t dir = (new_pos - old_pos > 0) ? 1 : -1;
    const double a = ACCELERATION * scaling;  // mm per s^2
    const double v_0 = MIN_SPEED * scaling;
    const double v_max = MAX_SPEED * scaling;
    // Assume target is far enough to accelerate to full speed
    const double t_accel = (v_max - v_0) / a;
    double S_accel = std::abs(v_0 * t_accel + a * std::pow(t_accel, 2) / 2);
    // Target isn't far enough
    if (S_accel > S / 2) {
        // Accelerate for only half of distance
        S_accel = S / 2;
    }
    setSpeed(v_0);
    std::thread stepping(&Axis::step, this,
                         static_cast<uint32_t>(std::ceil(S / step_length)));
    const auto dt = 1ms;
    const double dv = a * dt.count() / (1000ms).count();
    // Acceleration
    while (dir * (getPosition() - old_pos) < S_accel &&
           dir * (v_max - getSpeed()) > 0) {
        setSpeed(getSpeed() + dv);
        std::this_thread::sleep_for(dt);
    }
    // Constant speed
    while (dir * (new_pos - getPosition()) > S_accel) {
        std::this_thread::sleep_for(dt);
    }
    // Deceleration
    while (dir * (new_pos - getPosition()) > 0 && dir * (getSpeed() - v_0) > 0) {
        setSpeed(getSpeed() - dv);
        std::this_thread::sleep_for(dt);
    }
    stepping.join();
}

double Axis::getPosition() {
    std::shared_lock lck(position_mx);
    return position;
}

void Axis::setPosition(const double new_pos) {
    std::unique_lock lck(position_mx);
    position = new_pos;
}

void Axis::incPosition(const double inc) {
    std::unique_lock lck(position_mx);
    position += inc;
}

void Axis::step(uint32_t steps) {
    for (uint32_t i = 0; i < steps; i++) {
        // Stop when limit switch reached
        if (motor.getDirection() == Motor::NEGATIVE && negative.has_value() &&
            negative.value()) {
            break;
        }
        motor.step();
        incPosition(step_length * motor.getDirection());
    }
}

void Axis::setSpeed(double speed) {
    motor.setFrequency(std::abs(speed) / step_length);
    motor.setDirection(speed > 0 ? Motor::POSITIVE : Motor::NEGATIVE);
}

double Axis::getSpeed() const {
    double scalar = motor.getFrequency() * step_length;
    return scalar * static_cast<int8_t>(motor.getDirection());
}

}  // namespace pawnshop
