#pragma once
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include "limit_switch.hpp"
#include "motor.hpp"

namespace pawnshop {

class Axis {
public:
    /**
     * @param stepCount amount of steps in axis
     */
    explicit Axis(const double axisLength, const uint32_t stepCount,
                  const double minSpeed, const double maxSpeed,
                  const double axeleration, Motor&& motor,
                  LimitSwitch&& negative);
    Axis(const Axis&) = delete;
    Axis(Axis&&);
    Axis& operator=(const Axis&) = delete;
    Axis& operator=(Axis&&) = delete;
    void calibrate();
    /**
     * @param scaling scales speeds and acceleration to sync axes movement
     */
    void move(const double newPos, const double scaling);
    double getPosition();

private:
    const double MIN_SPEED;
    const double MAX_SPEED;
    const double ACCELERATION;
    Motor motor;
    const double axisLength;
    const double stepLength;
    std::optional<LimitSwitch> negative;
    std::timed_mutex positionMut;
    double position;
    void step(uint32_t steps);
    void setSpeed(double speed);
    void setPosition(const double newPos);
    void incPosition(const double inc);
    double getSpeed() const;
};

}  // namespace pawnshop
