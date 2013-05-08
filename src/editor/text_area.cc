#include "editor/text_area.h"
#include <cassert>
#include "wx/dcbuffer.h"
#include "wx/dcclient.h"
#include "wx/log.h"
#include "editor/compile_config.h"
#include "editor/text_window.h"
#include "editor/line_nr_area.h"
#include "editor/renderer.h"

namespace jil {
namespace editor {

#define kTextAreaStyle (wxBORDER_NONE | wxWANTS_CHARS)

BEGIN_EVENT_TABLE(TextArea, wxPanel)
EVT_PAINT(TextArea::OnPaint)
EVT_SIZE(TextArea::OnSize)
EVT_MOUSE_EVENTS(TextArea::OnMouseEvent)
EVT_KEY_DOWN(TextArea::OnKeyDown)
EVT_CHAR(TextArea::OnChar)
EVT_MOUSE_CAPTURE_LOST(TextArea::OnMouseCaptureLost)
END_EVENT_TABLE()

TextArea::TextArea() {
}

bool TextArea::Create(TextWindow* text_window, int id) {
  text_window_ = text_window;
  if (!wxPanel::Create(text_window, id, wxDefaultPosition, wxDefaultSize, kTextAreaStyle)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  return true;
}

void TextArea::ScrollWindow(int dx, int dy, const wxRect* rect) {
  wxPanel::ScrollWindow(dx, dy, rect);
  text_window_->line_nr_area()->ScrollWindow(0, dy, rect);
}

// Note: If wrap is on, line_start is the wrap line number.
//void TextArea::ScrollLineUp(int line_start, int line_count) {
//  int y = text_window_->line_height_ * (line_start - 1);
//  int scrolled_y = 0;
//  text_window_->CalcScrolledPosition(0, y, NULL, &scrolled_y);
//
//  wxRect scroll_rect(0,
//    scrolled_y,
//    GetClientRect().GetWidth(),
//    GetClientRect().GetHeight() - scrolled_y);
//
//  wxLogDebug("ScrollLineUp rect: %d-%d", scroll_rect.GetTop(), scroll_rect.GetBottom());
//  ScrollWindow(0, -text_window_->line_height_ * line_count, &scroll_rect);
//}

// Note: Call wxPanel::ScrollWindow() instead of TextArea::ScrollWindow()
// because the line number cannot simply scroll.
// TODO: wxGTK doesn't support scroll by a rectangle. It always scrolls the whole window.
// Note: If wrap is on, @line_start is the wrapped line number.
void TextArea::ScrollLineDown(int line_start, int line_count) {
  if (line_start <= 1) {
    wxPanel::ScrollWindow(0, text_window_->line_height_, NULL);
  } else {
    int y = text_window_->line_height_ * (line_start - 1);
    int scrolled_y = 0;
    text_window_->CalcScrolledPosition(0, y, NULL, &scrolled_y);
    wxRect scroll_rect(0,
                       scrolled_y,
                       GetClientRect().GetWidth(),
                       GetClientRect().GetHeight() - scrolled_y);

    wxLogDebug("ScrollLineDown rect: %d-%d", scroll_rect.GetTop(), scroll_rect.GetBottom());
    wxPanel::ScrollWindow(0,
                          text_window_->line_height_ * line_count,
                          &scroll_rect);
  }
}

void TextArea::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  text_window_->PrepareDC(dc);
  Renderer renderer(dc);
  text_window_->OnTextPaint(renderer);
}

void TextArea::OnSize(wxSizeEvent& evt) {
  text_window_->OnTextSize(evt);
  evt.Skip();
}

void TextArea::OnMouseEvent(wxMouseEvent& evt) {
  bool handled = text_window_->OnTextMouse(evt);
  if (!handled) { // If not handled, skip the event for default handling.
    evt.Skip();
  }
}

void TextArea::OnKeyDown(wxKeyEvent& evt) {
  bool handled = text_window_->OnTextKeyDown(evt);
  if (!handled) { // If not handled, skip the event for char event.
    evt.Skip();
  }
}

void TextArea::OnChar(wxKeyEvent& evt) {
  text_window_->OnTextChar(evt);
}

void TextArea::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  text_window_->OnMouseCaptureLost(evt);
}

} } // namespace jil::editor
