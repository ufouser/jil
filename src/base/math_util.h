#ifndef BASE_MATH_UTIL_H
#define BASE_MATH_UTIL_H
#pragma once

#include <cmath>

namespace base {\

const double PI = 3.14159265358979323846;
const double PI_1_2 = 1.57079632679489661923; // 1/2 PI
const double PI_3_2 = 4.71238898038468985769; // 3/2 PI

inline int Round(const double& d) {
  return static_cast<int>(std::floor(d + 0.5));
}

inline double RadianToDegree(double radians) {
  return radians * 180.0 / PI;
}

inline double DegreeToRadian(double degrees) {
  return degrees * PI / 180.0;
}

const double kRadian0 = 0.0;
const double kRadian90 = PI_1_2;
const double kRadian180 = PI;
const double kRadian270 = PI_3_2;

} // namespace base

#endif // BASE_MATH_UTIL_H
