#include "pawnshop/limit_switch.hpp"

namespace pawnshop {

LimitSwitch::LimitSwitch(gpiod::chip chip, size_t offset) {
    line = chip.get_line(offset);
    line.request(
        {"pawnshop-limitswitch", gpiod::line_request::DIRECTION_INPUT, 0});
}

LimitSwitch::operator bool() const { return !line.get_value(); }

}  // namespace pawnshop
