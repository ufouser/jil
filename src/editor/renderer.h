#ifndef JIL_EDITOR_RENDERER_H_
#define JIL_EDITOR_RENDERER_H_
#pragma once

#include <string>
#include "wx/gdicmn.h"

class wxDC;
class wxFont;
class wxColour;
class wxPen;
class wxBrush;

namespace jil {
namespace editor {

class Renderer {
public:
  explicit Renderer(wxDC& dc);
  ~Renderer();

  void SetFont(const wxFont& font, const wxColour& colour);
  void SetBrush(const wxBrush& brush);
  void SetPen(const wxPen& pen);

  void DrawText(const std::wstring& text, int x, int y, size_t off, size_t count = std::wstring::npos, int* w = NULL);

  void DrawText(const std::wstring& text, int x, int y, int* w = NULL) {
    DrawText(text, x, y, 0, text.size(), w);
  }

  void DrawLine(int x1, int y1, int x2, int y2);

  void DrawRectangle(int x, int y, int h, int w);
  void DrawRectangle(const wxRect& rect) {
    DrawRectangle(rect.x, rect.y, rect.width, rect.height);
  }

  void DrawWhiteSpaces(int x, int y, size_t count = 1, int* w = NULL);
  void DrawTabs(int x, int y, int w, int h);

private:
  wxDC& dc_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_RENDERER_H_
