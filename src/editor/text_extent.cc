#include "editor/text_extent.h"
#include "wx/dcmemory.h"
#include "wx/font.h"
#include "editor/tab.h"

namespace jil {
namespace editor {

TextExtent::TextExtent() {
  wxBitmap bmp(1, 1);
  dc_ = new wxMemoryDC(bmp);
}

TextExtent::~TextExtent() {
  delete dc_;
}

void TextExtent::SetFont(const wxFont& font) {
  dc_->SetFont(font);
}

wxCoord TextExtent::GetWidth(const std::wstring& text) {
  wxCoord x = 0;
  GetExtent(text, &x, NULL);
  return x;
}

wxCoord TextExtent::GetWidth(const std::wstring& text, int offset, int count) {
  std::wstring sub_text = text.substr(offset, count == kXCharEnd ? std::wstring::npos : count);
  wxCoord x = 0;
  GetExtent(sub_text, &x, NULL);
  return x;
}

void TextExtent::GetExtent(const std::wstring& text, wxCoord* x, wxCoord* y,
                           wxCoord* external_leading) {
  dc_->GetTextExtent(wxString(text.c_str()), x, y, 0, external_leading);
}

////////////////////////////////////////////////////////////////////////////////

// Binary search to get the char index.
// The range is STL-style: [begin, end).
// Return -1 if no match is found.
static
int __IndexChar(TextExtent& text_extent,
                int tab_stop,
                const std::wstring& line,
                int begin,
                int end,
                int client_x) {
  if (begin >= end) {
    return begin;
  }

  int m = begin + (end - begin) / 2;

  // Take unexpanded tab into account.
  std::wstring line_left = line.substr(0, m);
  TabbedLineFast(tab_stop, line_left);

  int width = text_extent.GetWidth(line_left);
  int m_char_width = text_extent.GetWidth(std::wstring(1, line[m]));

  if (std::abs(width - client_x) < m_char_width / 2) {
    return m;
  } else if (client_x > width) {
    return __IndexChar(text_extent, tab_stop, line, m + 1, end, client_x);
  } else { // client_x < width
    return __IndexChar(text_extent, tab_stop, line, begin, m, client_x);
  }
}

int IndexChar(TextExtent& text_extent, int tab_stop, const std::wstring& line, int client_x) {
  return __IndexChar(text_extent, tab_stop, line, 0, line.length(), client_x);
}

size_t TailorLabel(wxDC& dc, const wxString& label, int max_width) {
  int w = 0;
  for (size_t i = 0; i < label.size(); ++i) {
    dc.GetTextExtent(label.Mid(0, i), &w, NULL, NULL, NULL, NULL);
    if (w > max_width) {
      return i == 0 ? i : i - 1;
    }
  }
  return label.size();
}


} } // namespace jil::editor
