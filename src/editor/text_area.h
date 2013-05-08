#ifndef JIL_EDITOR_TEXT_AREA_H_
#define JIL_EDITOR_TEXT_AREA_H_
#pragma once

#include "wx/panel.h"
#include "base/compiler_specific.h"

namespace jil {
namespace editor {

class TextWindow;

class TextArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TextArea();
  bool Create(TextWindow* text_editor, wxWindowID id);

  // Override to scroll line nr area together with text area.
  virtual void ScrollWindow(int dx, int dy, const wxRect* rect) OVERRIDE;

  //void ScrollLineUp(int line_start, int line_count = 1);
  void ScrollLineDown(int line_start, int line_count = 1);

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);
  void OnMouseEvent(wxMouseEvent& evt);
  void OnKeyDown(wxKeyEvent& evt);
  void OnChar(wxKeyEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

private:
  TextWindow* text_window_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_AREA_H_
