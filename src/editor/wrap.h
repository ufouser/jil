#ifndef JIL_EDITOR_WRAP_H_
#define JIL_EDITOR_WRAP_H_
#pragma once

#include <vector>
#include <deque>
#include "editor/text_range.h"

namespace jil {
namespace editor {

class TextExtent;
class TextBuffer;

// About "wrap delta":
// = 0: Wrap is not changed
// < 0: Wrap is decreased
// > 0: Wrap is increased

// Wrap offsets.
// If a line "ABCDEFGHIJ" is wrapped as:
// ABC
// DEF
// GHI
// J
// The wrap offsets is: [3, 6, 9].
typedef std::vector<Coord> WrapOffsets;

WrapOffsets WrapLineByChar(const std::wstring& line, TextExtent* text_extent, Coord max_width);
// TODO
//WrapOffsets WrapLineByWord(const std::wstring& line, TextExtent* text_extent, Coord max_width);

// Wrap information for a text line.
class WrapInfo {
public:
  const WrapOffsets& offsets() const { return offsets_; }
  Coord WrapCount() const { return static_cast<Coord>(offsets_.size()); }

  // Wrap line according to the given max width.
  // Parameter @delta indicates the number of wraps increased (> 0) or decreased (< 0).
  // Returns if the wraps changes or not.
  bool Wrap(const std::wstring& line, TextExtent* text_extent, Coord max_width, int& delta);

  // Returns the number of wraps decreased.
  size_t Unwrap();

  // Get sub ranges (ranges of the sub lines).
  // Example:
  // Offsets: [3, 6, 9] -> Sub ranges: [<0, 3>, <3, 6>, <6, 9>, <9, npos/-1>]
  void GetSubRanges(std::vector<CharRange>& sub_ranges) const;

  // Example:
  // Given line "ABCDEFGHIJ" wrapped as:
  //       sub_ln
  // --------------
  // ABC      1
  // DEF      2
  // GHI      3
  // J        4
  // GetSubLine(4, ...) -> 2; sub_off: 3.
  // Return the wrap sub-line number.
  Coord GetSubLineNr(Coord x, size_t* sub_off = NULL) const;

  size_t GetOffset(Coord sub_ln) const {
    assert(sub_ln >= 1 && sub_ln <= WrapCount() + 1);
    return sub_ln == 1 ? 0 : offsets_[sub_ln - 1];
  }

private:
  WrapOffsets offsets_;
};

class WrapHelper {
public:
  WrapHelper(TextBuffer* buffer, TextExtent* text_extent);
  ~WrapHelper();

  void set_client_width(int client_width) {
    client_width_ = client_width;
  }

  const WrapInfo& GetWrapInfo(Coord ln) const {
    assert(ln <= static_cast<Coord>(wrap_infos_.size()));
    return wrap_infos_[ln - 1];
  }

  Coord WrapCount(Coord ln) const {
    return GetWrapInfo(ln).WrapCount();
  }

  // Return the wrap delta.
  int AddLineWrap(Coord ln);

  void RemoveLineWrap(Coord ln);

  // Return the wrap delta.
  int UpdateLineWrap(Coord ln);

  // Given client width as 3 * char_width.
  // Before wrap:
  //  2 DEFGHIJ
  // After wrap:
  //  2 DEF
  //    GHI
  //    J
  // WrappedLineCount(ln) returns 3.
  Coord SubLineCount(Coord ln) const {
    return WrapCount(ln) + 1;
  }

  // Given client width as 3 * char_width.
  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //    GHI
  //    J
  //  5 KLM
  //    N
  // WrappedLineCount() returns 6.
  Coord SubLineCount() const;

  // Wrap all lines.
  // Wrap delta indicates the number of wraps increased (> 0) or decreased (< 0).
  // Returns true if the wraps changes.
  bool Wrap(int& wrap_delta);

  // Unwrap all lines.
  // Wrap delta indicates the number of wraps increased (> 0) or decreased (< 0).
  // Returns true if the wraps changes.
  bool Unwrap(int& wrap_delta);

  Coord WrapLineNr(Coord unwrapped_ln) const;

  // TODO: Comments for sub_ln.
  Coord UnwrapLineNr(Coord wrapped_ln, int& sub_ln) const;

  // Wrap and UnWape line range.
  // Examples:
  // Given client width as 3 * char_width.
  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //  | GHI
  //  | J
  //  3 KLM
  //  | N
  // And we have:
  //  - Wrap([1, 1]) -> [1, 1]
  //  - Wrap([1, 2]) -> [1, 4]
  //  - Wrap([2, 3]) -> [2, 6]
  //  - Wrap([3, 3]) -> [5, 6]
  //  - Wrap([4, 5]) -> [0, 0]
  // Also we have:
  //  - Unwrap([1, 1]) -> [1, 1]
  //  - Unwrap([1, 2]) -> [1, 2]
  //  - Unwrap([1, 3]) -> [1, 2]
  //  - Unwrap([2, 4]) -> [2, 2]
  //  - Unwrap([2, 8]) -> [2, 3]
  //  - Unwrap([10, 20]) -> [0, 0]
  LineRange Wrap(const LineRange& line_range) const;
  LineRange Unwrap(const LineRange& wrapped_line_range) const;

private:
  TextBuffer* buffer_;
  TextExtent* text_extent_;
  int client_width_;

  typedef std::deque<WrapInfo> WrapInfos;
  WrapInfos wrap_infos_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_WRAP_H_
