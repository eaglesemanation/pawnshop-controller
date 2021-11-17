#pragma once

#include <array>
#include <mutex>
#include <shared_mutex>

#include "axis.hpp"
#include "vec.hpp"

namespace pawnshop {

class Rails {
public:
    Rails(std::array<Axis, 3>&& axes);
    Rails(const Rails&) = delete;
    Rails(Rails&&) = default;
    ~Rails() = default;
    void move(const pawnshop::vec::Vec3D newPos);
    pawnshop::vec::Vec3D getPos();

private:
    void calibrate();
    std::array<Axis, 3> axes;
};

}  // namespace pawnshop
