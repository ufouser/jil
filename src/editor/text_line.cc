#include "editor/text_line.h"
#include <cassert>
#include "base/foreach.h"
#include "editor/text_extent.h"
#include "editor/defs.h"
#include "editor/tab.h"

namespace jil {
namespace editor {

TextLine::TextLine(size_t id, const std::wstring& data)
    : id_(id), data_(data) {
}

wchar_t TextLine::GetChar(Coord index) const {
  if (index >= Length()) {
    return LF;
  }
  return data_[index];
}

Coord TextLine::TabbedLength(int tab_stop) const {
  return TabbedLineLength(tab_stop, data_);
}

void TextLine::InsertChar(Coord index, wchar_t c) {
  data_.insert(index, 1, c);
}

void TextLine::DeleteChar(Coord index, wchar_t* c) {
  if (c != NULL) {
    *c = data_[index];
  }
  data_.erase(index, 1);
}

void TextLine::InsertString(Coord index, const std::wstring& str) {
  data_.insert(index, str);
}

void TextLine::DeleteString(Coord index, size_t count, std::wstring* str) {
  if (str != NULL) {
    *str = data_.substr(index, count);
  }
  data_.erase(index, count);
}

void TextLine::Clear(std::wstring* line_data) {
  if (line_data != NULL) {
    line_data->clear();
    line_data->swap(data_);
  } else {
    data_.clear();
  }
}

void TextLine::Append(const std::wstring& str) {
  data_.append(str);
}

TextLine* TextLine::Split(Coord index, size_t line_id) {
  assert(index <= Length());

  TextLine* new_line = NULL;
  if (index == Length()) {
    new_line = new TextLine(line_id);
  } else {
    new_line = new TextLine(line_id, data_.substr(index));
    data_.erase(index);
  }

  return new_line;
}

} } // namespace jil::editor
