#ifndef JIL_EDITOR_GRAPHICS_H_
#define JIL_EDITOR_GRAPHICS_H_
#pragma once

#include <string>
#include "wx/gdicmn.h"

class wxFont;
class wxColour;
class wxPen;
class wxBrush;
class wxGraphicsContext;

namespace jil {\
namespace editor {\

class Graphics {
public:
  explicit Graphics(wxGraphicsContext* gc);
  ~Graphics();

  void SetFont(const wxFont& font, const wxColour& color);
  void SetBrush(const wxBrush& brush);
  void SetPen(const wxPen& pen);

  void DrawText(const wxString& text, int x, int y);
  void DrawLine(int x1, int y1, int x2, int y2);
  void DrawRectangle(int x, int y, int h, int w);

private:
  wxGraphicsContext* gc_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_GRAPHICS_H_
