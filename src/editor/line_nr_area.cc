#include "editor/line_nr_area.h"
#include "wx/dcbuffer.h"
#include "wx/log.h"
#include "editor/compile_config.h"
#include "editor/text_window.h"

namespace jil {
namespace editor {

BEGIN_EVENT_TABLE(LineNrArea, wxPanel)
EVT_PAINT(LineNrArea::OnPaint)
EVT_SIZE(LineNrArea::OnSize)
END_EVENT_TABLE()

LineNrArea::LineNrArea() {
}

bool LineNrArea::Create(TextWindow* text_window, wxWindowID id) {
  text_window_ = text_window;
  if (!wxPanel::Create(text_window, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_CUSTOM);

  return true;
}

void LineNrArea::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  text_window_->PrepareDC(dc);
  text_window_->OnLineNrPaint(dc);
}

void LineNrArea::OnSize(wxSizeEvent& evt) {
  text_window_->OnLineNrSize(evt);
  evt.Skip();
}

} } // namespace jil::editor
