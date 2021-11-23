#pragma once

#include <gpiod.hpp>
#include <toml++/toml_table.hpp>

namespace pawnshop {

struct LimitSwitchConfig {
    size_t pin;

    LimitSwitchConfig(const toml::table& table);
};

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

}  // namespace pawnshop
