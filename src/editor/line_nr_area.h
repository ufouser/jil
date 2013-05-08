#ifndef JIL_EDITOR_LINE_NR_AREA_H_
#define JIL_EDITOR_LINE_NR_AREA_H_
#pragma once

#include "wx/panel.h"

namespace jil {
namespace editor {

class TextWindow;

class LineNrArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  LineNrArea();
  bool Create(TextWindow* text_window, wxWindowID id);

  virtual bool AcceptsFocus() const { return false; }

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);

private:
  TextWindow* text_window_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_LINE_NR_AREA_H_
