#include "pawnshop/vec.hpp"

#include <cmath>
#include <stdexcept>

namespace pawnshop {
namespace vec {

double length(const Vec3D& v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

Vec3D normalize(const Vec3D& v) {
    double len = length(v);
    return {v[0] / len, v[1] / len, v[2] / len};
}

Vec3D operator-(const Vec3D& v) { return {-v[0], -v[1], -v[2]}; }

Vec3D operator+(const Vec3D& l, const Vec3D& r) {
    return {l[0] + r[0], l[1] + r[1], l[2] + r[2]};
}

Vec3D operator-(const Vec3D& l, const Vec3D& r) { return l + -r; }

Vec3D operator*(const Vec3D& v, const double scalar) {
    return {v[0] * scalar, v[1] * scalar, v[2] * scalar};
}

Vec3D operator/(const Vec3D& v, const double scalar) {
    return v * (1 / scalar);
}

Vec3D& operator+=(Vec3D& l, const Vec3D r) {
    l = l + r;
    return l;
}

Vec3D& operator-=(Vec3D& l, const Vec3D r) {
    l = l - r;
    return l;
}

std::ostream& operator<<(std::ostream& os, const Vec3D& v) {
    os << "[" << v[0] << ", " << v[1] << ", " << v[2] << "]";
    return os;
}

}  // namespace vec
}  // namespace pawnshop
