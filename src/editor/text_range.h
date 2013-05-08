#ifndef JIL_EDITOR_TEXT_RANGE_H_
#define JIL_EDITOR_TEXT_RANGE_H_
#pragma once

#include <vector>
#include <cassert>
#include "editor/text_point.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

// The range among a single line.
// [begin, end).
class CharRange {
public:
  CharRange(int begin = 0, int end = kXCharEnd);

  int begin() const { return begin_; }
  void set_begin(int begin) { begin_ = begin; }

  int end() const { return end_; }
  void set_end(int end) { end_ = end; }

  void Set(int begin, int end) {
    begin_ = begin;
    end_ = end;
  }

  Coord CharCount() const {
    return end_ == kXCharEnd ? kXCharEnd : (end_ - begin_);
  }

  CharRange Union(const CharRange& rhs);
  CharRange Intersect(const CharRange& rhs);

  bool IsEmpty() const {
    return end_ != kXCharEnd && begin_ >= end_;
  }

private:
  int begin_;
  int end_;
};

inline bool operator==(const CharRange& lhs, const CharRange& rhs) {
  return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();
}

inline bool operator!=(const CharRange& lhs, const CharRange& rhs) {
  return !(lhs == rhs);
}

////////////////////////////////////////////////////////////////////////////////

// [first, last] with first <= last.
class Range {
 public:
  explicit Range(Coord first, Coord last = 0) // TODO: remove default value 0.
      : first_(first), last_(last) {
    if (last_ < first_) {
      last_ = first_;
    }
  }

  Range(const Range& rhs)
      : first_(rhs.first_), last_(rhs.last_) {
  }

  Coord first() const { return first_; }
  void set_first(Coord first) { first_ = first; }

  Coord last() const { return last_; }
  void set_last(Coord last) { last_ = last; }

  Coord LineCount() const {
    return last_ + 1 - first_;
  }

  bool Has(Coord value) const {
    return value >= first_ && value <= last_;
  }

 protected:
  Coord first_;
  Coord last_;
};

class LineRange : public Range {
public:
  LineRange(Coord first = 0, Coord last = 0)
      : Range(first, last) {
  }

  bool IsEmpty() const {
    return first_ <= 0 || last_ <= 0 || first_ > last_;
  }
};

inline bool operator==(const Range& lhs, const Range& rhs) {
  return lhs.first() == rhs.first() && lhs.last() == rhs.last();
}

inline bool operator!=(const Range& lhs, const Range& rhs) {
  return !(lhs == rhs);
}

////////////////////////////////////////////////////////////////////////////////

// Range of text.
// [first_point, last_point] with first_point <= last_point.
class TextRange {
public:
  // Construct an invalid text range.
  TextRange() {
  }

  // Note: first_point <= last_point.
  TextRange(const TextPoint& first_point, const TextPoint& last_point)
      : first_point_(first_point), last_point_(last_point) {
    assert(first_point_ <= last_point_);
  }

  TextRange(const TextRange& rhs)
      : first_point_(rhs.first_point_), last_point_(rhs.last_point_) {
  }

  Coord LineCount() const { return last_point_.y + 1 - first_point_.y; }

  // TODO: Rename as IsEmpty
  bool Empty() const {
    return first_point_.y < 1 || last_point_.y < 1 || first_point_ >= last_point_;
  }

  bool Contain(const TextPoint& point) const {
    return point >= first_point_ && point < last_point_;
  }

  const TextPoint& first_point() const { return first_point_; }
  const TextPoint& last_point() const { return last_point_; }

  // Note: first_point <= last_point.
  void Set(const TextPoint& first_point, const TextPoint& last_point) {
    assert(first_point <= last_point);
    first_point_ = first_point;
    last_point_ = last_point;
  }

  void Reset() {
    first_point_.Set(0, 0); // TODO: -1 for x?
    last_point_.Set(0, 0);
  }

  // Get the char range of the given line.
  CharRange GetCharRange(Coord ln) const;

  // If the given line is in this text range.
  bool HasLine(Coord ln) const {
    return ln >= first_point_.y && ln <= last_point_.y;
  }

  // If this text range has any line.
  bool HasAnyLine() const {
    return first_point_.y > 0 && last_point_.y > 0 && first_point_.y <= last_point_.y;
  }

private:
  TextPoint first_point_;
  TextPoint last_point_;
};

inline bool operator==(const TextRange& lhs, const TextRange& rhs) {
  return lhs.first_point() == rhs.first_point() &&
         lhs.last_point() == rhs.last_point();
}

inline bool operator!=(const TextRange& lhs, const TextRange& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const TextRange& lhs, const TextRange& rhs) {
  return lhs.first_point().y < rhs.first_point().y;
}

////////////////////////////////////////////////////////////////////////////////

void DiffTextRangeByLine(const TextRange& old_range,
                         const TextRange& new_range,
                         std::vector<Coord>& diff_lines);

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_RANGE_H_
