#pragma once

#include <array>
#include <ostream>

namespace pawnshop {
namespace vec {
using Vec3D = std::array<double, 3>;

double length(const Vec3D& v);
Vec3D normalize(const Vec3D& v);

Vec3D operator-(const Vec3D& v);
Vec3D operator+(const Vec3D& l, const Vec3D& r);
Vec3D operator-(const Vec3D& l, const Vec3D& r);
Vec3D operator*(const Vec3D& v, const double scalar);
Vec3D operator/(const Vec3D& v, const double scalar);
Vec3D& operator+=(Vec3D& l, const Vec3D r);
Vec3D& operator-=(Vec3D& l, const Vec3D r);
std::ostream& operator<<(std::ostream& os, const Vec3D& v);
}  // namespace vec
}  // namespace pawnshop
