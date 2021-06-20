#pragma once

#include <gpiod.hpp>

namespace pawnshop {

class LimitSwitch {
public:
    LimitSwitch(gpiod::chip chip, size_t offset);
    LimitSwitch() = delete;
    LimitSwitch(const LimitSwitch&) = delete;
    LimitSwitch(LimitSwitch&&) = default;
    LimitSwitch& operator=(const LimitSwitch&) = delete;
    LimitSwitch& operator=(LimitSwitch&&) = delete;
    /**
     * @returns True if limit switch reached
     */
    operator bool() const;
private:
    gpiod::line line;
};

}