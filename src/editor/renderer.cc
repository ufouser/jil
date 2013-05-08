#include "editor/renderer.h"
#ifdef __WXMSW__
#include <Windows.h>
#endif __WXMSW__
#include "wx/dc.h"

namespace jil {
namespace editor {

Renderer::Renderer(wxDC& dc)
    : dc_(dc) {
}

Renderer::~Renderer() {
}

void Renderer::SetFont(const wxFont& font, const wxColour& colour) {
  dc_.SetFont(font);
  dc_.SetTextForeground(colour);
}

void Renderer::SetBrush(const wxBrush& brush) {
  dc_.SetBrush(brush);
}

void Renderer::SetPen(const wxPen& pen) {
  dc_.SetPen(pen);
}

void Renderer::DrawText(const std::wstring& text, int x, int y, size_t off, size_t count, int* w) {
  if (text.empty()) {
    if (w != NULL) {
      *w = 0;
    }
    return;
  }

  assert(off < text.size());

#ifdef __WXMSW__
  // Avoid string copy.
  HDC hdc = dc_.GetHDC();

  const wxColour& fg = dc_.GetTextForeground();
  assert(fg.IsOk());
  COLORREF old_fg = ::SetTextColor(hdc, fg.GetPixel());

  int old_bg_mode = ::SetBkMode(hdc, dc_.GetBackgroundMode() == wxBRUSHSTYLE_TRANSPARENT ? TRANSPARENT : OPAQUE);

  if (count == std::wstring::npos) {
    count = text.size() - off;
  }
  ::ExtTextOut(hdc, x, y, 0, NULL, text.c_str() + off, count, NULL);

  if (w != NULL) {
    SIZE size;
    ::GetTextExtentPoint32(hdc, text.c_str() + off, count, &size);
    *w = size.cx;
  }

  ::SetTextColor(hdc, old_fg);
  ::SetBkMode(hdc, old_bg_mode);
#else
  wxString wx_text(text.substr(off, count));
  dc_.DrawText(wx_text, x, y);
  if (w != NULL) {
    dc_.GetTextExtent(wx_text, w, NULL);
  }
#endif // __WXMSW__
}

void Renderer::DrawLine(int x1, int y1, int x2, int y2) {
  dc_.DrawLine(x1, y1, x2, y2);
}

void Renderer::DrawRectangle(int x, int y, int h, int w) {
  dc_.DrawRectangle(x, y, h, w);
}

void Renderer::DrawWhiteSpaces(int x, int y, size_t count, int* w) {
  int space_w = 0;
  int space_h = 0;
  dc_.GetTextExtent(_T(" "), &space_w, &space_h);

  int space_x = x + space_w / 2;
  int space_y = y + space_h / 2;
  for (size_t i = 0; i < count; ++i) {
    dc_.DrawRectangle(space_x, space_y, 2, 2);
    space_x += space_w;
  }

  if (w != NULL) {
    *w = space_w * count;
  }
}

void Renderer::DrawTabs(int x, int y, int w, int h) {
  int tab_y = y + h / 2;
  int r = x + w - 1;
  int size = 4; // TODO: Adjust the arrow according to char height.
  dc_.DrawLine(x + 1, tab_y, r + 1, tab_y); // Arrow head.
  dc_.DrawLine(r - size, tab_y - size, r, tab_y);
  dc_.DrawLine(r - size, tab_y + size, r, tab_y);
}

} } // namespace jil::editor
