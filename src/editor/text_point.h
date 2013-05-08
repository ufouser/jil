#ifndef JIL_EDITOR_TEXT_POINT_H_
#define JIL_EDITOR_TEXT_POINT_H_
#pragma once

#include <limits>
#include <cassert>

namespace jil {\
namespace editor {\

////////////////////////////////////////////////////////////////////////////////

// Coordinate for text by char and line.
// Note: Don't use unsigned type (e.g., size_t) for Coord!
typedef int Coord;
#define kCoordMax INT_MAX
#define kUCoordMax UINT_MAX

const Coord kXCharEnd = -1; // TODO: Rename

// Explicitly cast Coord, e.g., from size_t.
template <typename T>
inline Coord CoordCast(T value) {
  return static_cast<Coord>(value);
}

template <>
inline Coord CoordCast<size_t>(size_t value) {
  typedef unsigned int UCoord;
  assert(value <= kUCoordMax >> 1); // TODO
  return static_cast<Coord>(value);
}

////////////////////////////////////////////////////////////////////////////////

class TextPoint {
public:
  TextPoint(Coord x_ = 0, Coord y_ = 1)
    : x(x_), y(y_) {
  }

  void Set(Coord x_, Coord y_) {
    x = x_;
    y = y_;
  }

  bool Valid() const {
    return x >= 0 && y > 0;
  }

  Coord x; // 0-based X coordinate by char.
  Coord y; // 1-based Y coordinate by line.
};

inline bool operator==(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const TextPoint& lhs, const TextPoint& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs.y < rhs.y ||
         (lhs.y == rhs.y &&
          (rhs.x == kXCharEnd ? lhs.x != kXCharEnd : lhs.x < rhs.x)); // TODO
}

inline bool operator>(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs.y > rhs.y ||
         (lhs.y == rhs.y &&
          (lhs.x == kXCharEnd ? rhs.x != kXCharEnd : lhs.x > rhs.x)); // TODO
}

inline bool operator<=(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs < rhs || lhs == rhs;
}

inline bool operator>=(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs > rhs || lhs == rhs;
}

inline TextPoint operator+(const TextPoint& lhs, const TextPoint& rhs) {
  return TextPoint(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline TextPoint operator-(const TextPoint& lhs, const TextPoint& rhs) {
  return TextPoint(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline TextPoint& operator+=(TextPoint& lhs, const TextPoint& rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_POINT_H_
