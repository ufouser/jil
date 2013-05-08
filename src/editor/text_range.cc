#include "editor/text_range.h"
#include <cassert>
#include "base/foreach.h"

namespace jil {
namespace editor {

CharRange::CharRange(Coord begin, Coord end)
    : begin_(begin), end_(end) {
}

CharRange CharRange::Union(const CharRange& rhs) {
  CharRange result;
  result.begin_ = std::min(begin_, rhs.begin_);
  if (end_ == kXCharEnd) {
    result.end_ = kXCharEnd;
  } else {
    if (rhs.end_ == kXCharEnd) {
      result.end_ = kXCharEnd;
    } else {
      result.end_ = std::max(end_, rhs.end_);
    }
  }
  return result;
}

CharRange CharRange::Intersect(const CharRange& rhs) {
  CharRange result;
  result.begin_ = std::max(begin_, rhs.begin_);
  if (end_ == kXCharEnd) {
    result.end_ = rhs.end_;
  } else {
    if (rhs.end_ == kXCharEnd) {
      result.end_ = end_;
    } else {
      result.end_ = std::min(end_, rhs.end_);
    }
  }
  if (result.end_ != kXCharEnd && result.end_ < result.begin_) {
    result.end_ = result.begin_;
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

CharRange TextRange::GetCharRange(Coord ln) const {
  assert(HasLine(ln));

  if (first_point_.y == last_point_.y) { // Single line.
    return CharRange(first_point_.x, last_point_.x);
  } else if (ln == first_point_.y) {
    return CharRange(first_point_.x, kXCharEnd);
  } else if (ln == last_point_.y) {
    return CharRange(0, last_point_.x);
  } else { // Middle line.
    return CharRange(0, kXCharEnd);
  }
}

////////////////////////////////////////////////////////////////////////////////

void DiffTextRangeByLine(const TextRange& old_range,
                         const TextRange& new_range,
                         std::vector<Coord>& diff_lines) {
  if (!old_range.HasAnyLine()) {
    if (!new_range.HasAnyLine()) {
      return;
    } else {
      diff_lines.resize(new_range.LineCount());
      size_t i = 0;
      for (Coord ln = new_range.first_point().y;
           ln <= new_range.last_point().y;
           ++i, ++ln) {
        diff_lines[i] = ln;
      }
      return;
    }
  } else { // Old range is valid.
    if (!new_range.HasAnyLine()) {
      diff_lines.resize(old_range.LineCount());
      size_t i = 0;
      for (Coord ln = old_range.first_point().y;
           ln <= old_range.last_point().y;
           ++i, ++ln) {
        diff_lines[i] = ln;
      }
      return;
    }
  }

  Coord i = old_range.first_point().y;
  Coord j = new_range.first_point().y;
  for (; i <= old_range.last_point().y && j <= new_range.last_point().y;) {
    if (i == j) {
      if (old_range.GetCharRange(i) != new_range.GetCharRange(j)) {
        diff_lines.push_back(i);
      }
      ++i;
      ++j;
    } else if (i < j) {
      diff_lines.push_back(i);
      ++i;
    } else { // j < i
      diff_lines.push_back(j);
      ++j;
    }
  }

  if (i <= old_range.last_point().y) {
    for (; i <= old_range.last_point().y; ++i) {
      diff_lines.push_back(i);
    }
  } else if (j <= new_range.last_point().y) {
    for (; j <= new_range.last_point().y; ++j) {
      diff_lines.push_back(j);
    }
  }
}

} } // namespace jil::editor
