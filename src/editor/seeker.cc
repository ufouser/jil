#include "editor/seeker.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

namespace {

// Compound operator should be considered as a single word.
// This is a sample table for C/C++. Different languages have different
// compound operators.
const std::wstring compound_operators[] = {
  L"==",
  L"<=",
  L">=",
  L"!=",
  L"+=",
  L"-=",
  L"*=",
  L"/=",
  L"&=",
  L"|=",
  L"&&",
  L"||",
  L"<<",
  L">>",
  L"++",
  L"--",
  L"::",
};

// TODO
bool IsOperator(wchar_t c) {
  return (c >= 0x21 && c <= 0x2f) ||
         (c >= 0x3a && c <= 0x40) ||
         (c >= 0x5b && c <= 0x5e) || // Excluding _ and `
         (c >= 0x7b && c <= 0x7e);
}

size_t IndexWordAfterOperator(const std::wstring& line_data, size_t i) {
  assert(i < line_data.length());
  assert(IsOperator(line_data[i]));

  // Back to the first operator of this operator group if any.
  size_t j = i;
  for (; j > 0; --j) {
    if (!IsOperator(line_data[j])) {
      ++j; // Now j is the index of the first operator.
      break;
    }
  }

  // Now check compound operators from index j.
  size_t co_count = sizeof(compound_operators)/sizeof(std::wstring);
  size_t line_length = line_data.size();

  bool is_compound = false;
  while (j <= i) {
    size_t co_i = 0;
    for (; co_i < co_count; ++co_i) {
      const std::wstring& co = compound_operators[co_i];
      if (j + co.length() <= line_length) {
        if (wcsncmp(co.c_str(), &line_data[j], co.length()) == 0) {
          j += co.length();
          is_compound = true;
          break;
        }
      }

    }
    if (co_i == co_count) { // No compound operator matches.
      // The operator at index j is a single word.
      ++j;
      is_compound = false;
    }
  }

  if (is_compound) {
    return j;
  } else {
    return i + 1;
  }
}

size_t IndexWordBeforeOperator(const std::wstring& line_data, size_t i) {
  assert(i < line_data.length());
  assert(IsOperator(line_data[i]));

  if (i == 0) {
    return i;
  }

  // Back to the first operator of this operator group if any.
  size_t j = i;
  for (; j > 0; --j) {
    if (!IsOperator(line_data[j])) {
      ++j; // Now j is the index of the first operator.
      break;
    }
  }

  // Now check compound operators from index j.
  size_t co_count = sizeof(compound_operators)/sizeof(std::wstring);
  size_t line_length = line_data.size();

  size_t compound_len = 0;
  while (j < i) {
    size_t co_i = 0;
    for (; co_i < co_count; ++co_i) {
      const std::wstring& co = compound_operators[co_i];
      if (j + co.length() <= line_length) {
        if (wcsncmp(co.c_str(), &line_data[j], co.length()) == 0) {
          j += co.length();
          compound_len = co.length();
          break;
        }
      }
    }
    if (co_i == co_count) { // No compound operator matches.
      // The operator at index j is a single word.
      ++j;
      compound_len = 0;
    }
  }

  if (compound_len > 0) {
    if (j == i) {
      return j;
    } else {
      return j - compound_len;
    }
  } else {
    return i;
  }
}

inline bool IsWhiteSpace(wchar_t c) {
  return c == kSpaceChar || c == kTabChar;
}

size_t SkipWhiteSpaces(const std::wstring& line_data, size_t i) {
  // Note: Don't assert i < line_data.length().
  while (i < line_data.size() && IsWhiteSpace(line_data[i])) {
    ++i;
  }
  return i;
}

} // namespace


size_t IndexWordBegin(const std::wstring& line_data, size_t i, bool inc_ws) {
  assert(i < line_data.size());

  if (IsWhiteSpace(line_data[i])) {
    for (; i > 0; --i) {
      if (!IsWhiteSpace(line_data[i - 1])) {
        break;
      }
    }
    if (inc_ws && i > 0) {
      i = IndexWordBegin(line_data, i - 1, inc_ws);
    }
  } else {
    if (IsOperator(line_data[i])) {
      i = IndexWordBeforeOperator(line_data, i);
    } else {
      for (; i > 0; --i) {
        wchar_t c = line_data[i - 1];
        if (IsWhiteSpace(c) || IsOperator(c)) {
          break;
        }
      }
    }
  }

  return i;
}

size_t IndexWordEnd(const std::wstring& line_data, size_t i, bool inc_ws) {
  assert(i < line_data.size());

  if (IsWhiteSpace(line_data[i])) { // Current char is white space.
    i = SkipWhiteSpaces(line_data, i + 1);
  } else if (IsOperator(line_data[i])) { // Current char is operator.
    i = IndexWordAfterOperator(line_data, i);
    if (inc_ws) {
      // Skip the white spaces after this operator.
      i = SkipWhiteSpaces(line_data, i);
    }
  } else {
    for (++i; i < line_data.size(); ++i) {
      if (IsWhiteSpace(line_data[i])) {
        if (inc_ws) {
          i = SkipWhiteSpaces(line_data, i);
        }
        break;
      } else if (IsOperator(line_data[i])) {
        break;
      }
    }
  }

  return i;
}

namespace {\

////////////////////////////////////////////////////////////////////////////////

TextPoint SeekPrevChar(const TextBuffer* buffer, const TextPoint& point) {
  if (point.x > 0) {
    return TextPoint(point.x - 1, point.y);
  } else {
    if (point.y > 1) {
      return TextPoint(buffer->LineLength(point.y - 1), point.y - 1);
    } else {
      return point;
    }
  }
}

TextPoint SeekNextChar(const TextBuffer* buffer, const TextPoint& point) {
  if (point.x < buffer->LineLength(point.y)) {
    return TextPoint(point.x + 1, point.y);
  } else {
    if (point.y < buffer->LineCount()) {
      return TextPoint(0, point.y + 1);
    } else {
      return point;
    }
  }
}

TextPoint SeekPrevWord(const TextBuffer* buffer, const TextPoint& point) {
  assert(point.y <= buffer->LineCount());

  const Coord line_length = buffer->LineLength(point.y);
  assert(point.x <= line_length);

  TextPoint begin_point;

  if (point.x == 0) {
    if (point.y == 1) { // No previous line.
      begin_point = point;
    } else {
      begin_point.y = point.y - 1;
      begin_point.x = buffer->LineLength(begin_point.y);
      if (begin_point.x > 0) { // Previous line is not empty.
        const std::wstring& line_data = buffer->GetLine(begin_point.y).data();
        begin_point.x = IndexWordBegin(line_data, begin_point.x - 1, true);
      }
    }
  } else { // point.x > 0
    begin_point.y = point.y;
    const std::wstring& line_data = buffer->GetLine(point.y).data();
    size_t i = IndexWordBegin(line_data, point.x - 1, true);
    begin_point.x = i;
  }

  return begin_point;
}

TextPoint SeekNextWord(const TextBuffer* buffer, const TextPoint& point) {
  assert(point.y <= buffer->LineCount());

  const Coord line_length = buffer->LineLength(point.y);
  assert(point.x <= line_length);

  TextPoint end_point;

  if (point.x == line_length) {
    if (point.y == buffer->LineCount()) { // No next line.
      end_point = point;
    } else {
      end_point.Set(0, point.y + 1);
      const std::wstring& line_data = buffer->GetLine(end_point.y).data();
      end_point.x = SkipWhiteSpaces(line_data, 0);
    }
  } else { // point.x < line_length
    const std::wstring& line_data = buffer->GetLine(point.y).data();
    size_t i = IndexWordEnd(line_data, point.x, true);
    end_point.Set(i, point.y);
  }

  return end_point;
}

TextPoint SeekWordBegin(const TextBuffer* buffer, const TextPoint& point) {
  const TextLine& line = buffer->GetLine(point.y);
  if (line.Length() == 0) {
    return point;
  }

  assert(point.x <= line.Length());

  if (point.x == 0) {
    return point;
  }

  if (point.x == line.Length()) {
    return TextPoint(IndexWordBegin(line.data(), point.x - 1, false), point.y);
  } else {
    return TextPoint(IndexWordBegin(line.data(), point.x, false), point.y);
  }
}

TextPoint SeekWordEnd(const TextBuffer* buffer, const TextPoint& point) {
  const TextLine& line = buffer->GetLine(point.y);
  if (line.Length() == 0) {
    return point;
  }

  assert(point.x <= line.Length());

  if (point.x == line.Length()) {
    return point;
  }

  return TextPoint(IndexWordEnd(line.data(), point.x, false), point.y);
}

TextPoint SeekLineBegin(const TextBuffer* buffer, const TextPoint& point) {
  wxUnusedVar(buffer);
  return TextPoint(0, point.y);
}

TextPoint SeekLineEnd(const TextBuffer* buffer, const TextPoint& point) {
  return TextPoint(buffer->LineLength(point.y), point.y);
}

TextPoint SeekPrevLine(const TextBuffer* buffer, const TextPoint& point) {
  wxUnusedVar(buffer);
  if (point.y <= 1) {
    return point;
  }
  return TextPoint(point.x, point.y - 1);
}

TextPoint SeekNextLine(const TextBuffer* buffer, const TextPoint& point) {
  if (point.y >= buffer->LineCount()) {
    return point;
  }
  return TextPoint(point.x, point.y + 1);
}


#define DEFINE_COUNTED_SEEK(seek_func)\
TextPoint seek_func(const TextBuffer* buffer, const TextPoint& point, size_t count) {\
  TextPoint seek_point = point;\
  for (size_t c = 0; c < count; ++c) {\
    seek_point = seek_func(buffer, seek_point);\
  }\
  return seek_point;\
}

DEFINE_COUNTED_SEEK(SeekPrevChar)
DEFINE_COUNTED_SEEK(SeekNextChar)
DEFINE_COUNTED_SEEK(SeekPrevWord)
DEFINE_COUNTED_SEEK(SeekNextWord)
DEFINE_COUNTED_SEEK(SeekWordBegin)
DEFINE_COUNTED_SEEK(SeekWordEnd)
DEFINE_COUNTED_SEEK(SeekLineBegin)
DEFINE_COUNTED_SEEK(SeekLineEnd)
DEFINE_COUNTED_SEEK(SeekPrevLine)
DEFINE_COUNTED_SEEK(SeekNextLine)

} // namespace

TextPoint Seek(const TextBuffer* buffer,
               const TextPoint& point,
               TextUnit text_unit,
               SeekType seek_type,
               size_t count) {
  switch (text_unit) {
    case kChar:
      if (seek_type == kPrev) {
        return SeekPrevChar(buffer, point, count);
      } else if (seek_type == kNext) {
        return SeekNextChar(buffer, point, count);
      }

      break;

    case kWord:
      if (seek_type == kBegin) {
        return SeekWordBegin(buffer, point, count);
      } else if (seek_type == kEnd) {
        return SeekWordEnd(buffer, point, count);
      } else if (seek_type == kPrev) {
        return SeekPrevWord(buffer, point, count);
      } else if (seek_type == kNext) {
        return SeekNextWord(buffer, point, count);
      }

      break;

    case kLine:
      if (seek_type == kBegin) {
        return SeekLineBegin(buffer, point, count);
      } else if (seek_type == kEnd) {
        return SeekLineEnd(buffer, point, count);
      } else if (seek_type == kPrev) {
        return SeekPrevLine(buffer, point, count);
      } else if (seek_type == kNext) {
        return SeekNextLine(buffer, point, count);
      }

      break;

    case kBuffer:
      if (seek_type == kBegin) {
        return TextPoint(0, 1);
      } else if (seek_type == kEnd) {
        Coord line_count = buffer->LineCount();
        return TextPoint(buffer->LineLength(line_count), line_count);
      }
      break;
  }

  return point;
}

} } // namespace jil::editor
