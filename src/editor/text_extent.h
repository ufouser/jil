#ifndef JIL_EDITOR_TEXT_EXTENT_H_
#define JIL_EDITOR_TEXT_EXTENT_H_
#pragma once

#include <string>
#include <vector>
#include "wx/defs.h"
#include "wx/string.h"
#include "editor/text_point.h"

class wxFont;
class wxDC;
class wxMemoryDC;

namespace jil {
namespace editor {

// Text extent used to calculate the text size.
class TextExtent {
public:
  TextExtent();
  ~TextExtent();

  void SetFont(const wxFont& font);

  wxCoord GetWidth(const std::wstring& text);
  wxCoord GetWidth(const std::wstring& text, int offset, int count = kXCharEnd);

  void GetExtent(const std::wstring& text,
                 wxCoord* x,
                 wxCoord* y,
                 wxCoord* external_leading = NULL);

private:
  wxMemoryDC* dc_;
};

// Binary search to get the char index with the given client x coordinate.
// Return -1 if not found.
int IndexChar(TextExtent& text_extent, int tab_stop, const std::wstring& line, int client_x);

size_t TailorLabel(wxDC& dc, const wxString& label, int max_width);

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_EXTENT_H_
