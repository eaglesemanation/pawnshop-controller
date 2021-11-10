#include "pawnshop/axis.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

namespace pawnshop {

Axis::Axis(const double axisLength, const uint32_t stepCount,
           const double minSpeed, const double maxSpeed,
           const double acceleration, Motor &&motor, LimitSwitch &&negative)
    : motor(std::move(motor)),
      negative(std::move(negative)),
      axisLength(axisLength),
      stepLength(axisLength / stepCount),
      MIN_SPEED(minSpeed),
      MAX_SPEED(maxSpeed),
      ACCELERATION(acceleration) {}

Axis::Axis(Axis &&src)
    : motor(std::move(src.motor)),
      negative(std::move(src.negative)),
      axisLength(src.axisLength),
      stepLength(src.stepLength),
      MIN_SPEED(src.MIN_SPEED),
      MAX_SPEED(src.MAX_SPEED),
      ACCELERATION(src.ACCELERATION) {}

void Axis::calibrate() {
    // TODO: Calibration for case with 2 limit switches
    setSpeed(-MIN_SPEED);
    while (negative.has_value() && !negative.value()) motor.step();
    setPosition(0.0);
}

void Axis::move(const double newPos, const double scaling) {
    const double oldPos = getPosition();
    const double S = std::abs(newPos - oldPos);
    const int8_t dir = (newPos - oldPos > 0) ? 1 : -1;
    const double a = ACCELERATION * scaling;  // mm per s^2
    const double v0 = MIN_SPEED * scaling;
    const double vMax = MAX_SPEED * scaling;
    // Assume target is far enough to accelerate to full speed
    const double tAccel = (vMax - v0) / a;
    double SAccel = std::abs(v0 * tAccel + a * std::pow(tAccel, 2) / 2);
    // Target isn't far enough
    if (SAccel > S / 2) {
        // Accelerate for only half of distance
        SAccel = S / 2;
    }
    setSpeed(v0);
    std::thread stepping(&Axis::step, this,
                         static_cast<uint32_t>(std::ceil(S / stepLength)));
    const auto dt = 1ms;
    const double dv = a * dt.count() / (1000ms).count();
    // Acceleration
    while (dir * (getPosition() - oldPos) < SAccel &&
           dir * (vMax - getSpeed()) > 0) {
        setSpeed(getSpeed() + dv);
        std::this_thread::sleep_for(dt);
    }
    // Constant speed
    while (dir * (newPos - getPosition()) > SAccel) {
        std::this_thread::sleep_for(dt);
    }
    // Deceleration
    while (dir * (newPos - getPosition()) > 0 && dir * (getSpeed() - v0) > 0) {
        setSpeed(getSpeed() - dv);
        std::this_thread::sleep_for(dt);
    }
    stepping.join();
}

double Axis::getPosition() {
    std::shared_lock lck(positionMut);
    return position;
}

void Axis::setPosition(const double newPos) {
    std::unique_lock lck(positionMut);
    position = newPos;
}

void Axis::incPosition(const double inc) {
    std::unique_lock lck(positionMut);
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
        incPosition(stepLength * motor.getDirection());
    }
}

void Axis::setSpeed(double speed) {
    motor.setFrequency(std::abs(speed) / stepLength);
    motor.setDirection(speed > 0 ? Motor::POSITIVE : Motor::NEGATIVE);
}

double Axis::getSpeed() const {
    double scalar = motor.getFrequency() * stepLength;
    return scalar * static_cast<int8_t>(motor.getDirection());
}

}  // namespace pawnshop
