#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <gpiod.hpp>
#include <toml++/toml_table.hpp>

#include "limit_switch.hpp"

namespace pawnshop {

struct MotorConfig {
    size_t clock_pin;
    size_t direction_pin;
    bool counter_clockwire;

    MotorConfig(const toml::table& table);
};

class Motor {
    explicit Motor(const gpiod::chip gpio_chip, const size_t clock_line_offset,
                   const size_t dir_line_offset, bool inverted);
    explicit Motor(gpiod::line clock_line, gpiod::line dir_line,
                   bool inverted = false);

public:
    explicit Motor(gpiod::chip chip, const std::unique_ptr<MotorConfig> conf);
    enum Direction : int8_t { NEGATIVE = -1, POSITIVE = 1 };
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
    gpiod::line clock_line, dir_line;
    Direction dir;
    const bool inverted;
    bool stopped = true;
    std::atomic<std::chrono::nanoseconds> period;
    void setPeriod(const std::chrono::nanoseconds period);
};

}  // namespace pawnshop
