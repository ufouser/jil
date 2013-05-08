#ifndef JIL_EDITOR_TEXT_LINE_H_
#define JIL_EDITOR_TEXT_LINE_H_
#pragma once

#include <string>
#include "editor/text_point.h"

namespace jil {
namespace editor {

class TextLine {
public:
  explicit TextLine(size_t id, const std::wstring& data = L"");

  size_t id() const { return id_; }

  const std::wstring& data() const { return data_; }

  wchar_t GetChar(Coord index) const;

  Coord Length() const { return CoordCast(data_.length()); }

  // Line length with tabs expanded.
  Coord TabbedLength(int tab_stop) const;

  void InsertChar(Coord index, wchar_t c);
  void DeleteChar(Coord index, wchar_t* c = NULL);

  void InsertString(Coord index, const std::wstring& str);
  void DeleteString(Coord index, size_t count, std::wstring* str = NULL);

  void Append(const std::wstring& str);

  void Clear(std::wstring* line_data = NULL);

  // Split at the given index and return the new line.
  TextLine* Split(Coord index, size_t line_id);

private:
  size_t id_;
  std::wstring data_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_LINE_H_
