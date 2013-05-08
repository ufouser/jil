#ifndef JIL_EDITOR_SEEKER_H_
#define JIL_EDITOR_SEEKER_H_
#pragma once

// Seek the text buffer from a given point.

#include <string>
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/text_point.h"

namespace jil {\
namespace editor {\

class TextBuffer;

size_t IndexWordBegin(const std::wstring& line_data, size_t i, bool inc_ws);
size_t IndexWordEnd(const std::wstring& line_data, size_t i, bool inc_ws);

enum SeekType {
  kWhole = 0,
  kBegin,
  kEnd,
  kPrev,
  kNext
};

TextPoint Seek(const TextBuffer* buffer, const TextPoint& point, TextUnit text_unit, SeekType seek_type, size_t count = 1);

} } // namespace jil::editor

#endif // JIL_EDITOR_SEEKER_H_
