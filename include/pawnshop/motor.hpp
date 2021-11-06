#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <gpiod.hpp>

#include "limit_switch.hpp"

namespace pawnshop {

class Motor {
public:
    enum Direction : int8_t { NEGATIVE = -1, POSITIVE = 1 };
    explicit Motor(const gpiod::chip gpioChip, const uint8_t clockLineOffset,
                   const uint8_t dirLineOffset, bool inverted = false);
    explicit Motor(gpiod::line clockLine, gpiod::line dirLine,
                   bool inverted = false);
    Motor() = delete;
    Motor(const Motor&) = delete;
    Motor(Motor&&);
    Motor& operator=(const Motor&) = delete;
    Motor& operator=(Motor&&) = delete;
    int16_t step() const;
    void setDirection(const Direction dir);
    Direction getDirection() const;
    void setFrequency(const uint32_t hz);
    int32_t getFrequency() const;

private:
    gpiod::line clockLine, dirLine;
    Direction dir;
    const bool inverted;
    bool stopped = true;
    std::atomic<std::chrono::nanoseconds> period;
    void setPeriod(const std::chrono::nanoseconds period);
};

}  // namespace pawnshop
