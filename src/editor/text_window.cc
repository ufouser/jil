#include "editor/text_window.h"
#include <cassert>
#include <cmath>
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "wx/caret.h"
#include "wx/timer.h"
#include "wx/log.h"
#include "base/foreach.h"
#include "base/string_util.h"
#include "base/math_util.h"
#include "editor/style.h"
#include "editor/binding.h"
#include "editor/line_nr_area.h"
#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/wrap.h"
#include "editor/tab.h"
#include "editor/text_extent.h"
#include "editor/renderer.h"
#include "editor/edit_action.h"
#include "editor/file_type.h"

// When to update virtual size?
// - OnTextSize: Because virtual size depends on client size of text area.
// - OnLineAdded, OnLineDeleted
// - Text font is changed
// - Wrap changes

namespace jil {\
namespace editor {\

////////////////////////////////////////////////////////////////////////////////

// TODO: Configurable
#define kMarginTop 3
#define kMarginBottom 0
#define kTextMarginLeft 0
#define kTextMarginBottom 0
#define kLineNrPadding 15
#define kLineNrPaddingRight 6
#define kLinePaddingY 1
#define kCaretWidth 1

//------------------------------------------------------------------------------

IMPLEMENT_CLASS(TextWindow, wxScrolledWindow)

BEGIN_EVENT_TABLE(TextWindow, wxScrolledWindow)
EVT_SIZE          (TextWindow::OnSize)
EVT_SET_FOCUS     (TextWindow::OnSetFocus)
END_EVENT_TABLE()

//------------------------------------------------------------------------------

TextWindow::TextWindow(TextBuffer* buffer)
    : buffer_(buffer)
    , buffer_modified_(buffer->modified())
    , options_(buffer->file_type()->options())
    , style_(NULL)
    , binding_(NULL)
    , line_nr_width_(0)
    , char_width_(0)
    , char_height_(0)
    , line_height_(0)
    , text_width_(0)
    , text_height_(0)
    , caret_point_(0, 1)
    , v_caret_coord_x_(0)
    , wrapped_(options_.wrap)
    , wrap_helper_(NULL)
    , down_modifiers_(0)
    , dragged_(false)
    , scroll_timer_(NULL)
    , scroll_dir_(kScrollDown)
    , select_dir_(kForward) {

  line_nr_area_ = new LineNrArea;
  text_area_ = new TextArea;
  status_line_ = new StatusLine;

  text_extent_ = new TextExtent;
}

bool TextWindow::Create(wxWindow* parent, int id) {
  assert(theme_);
  assert(style_ != NULL);
  assert(binding_ != NULL);

  status_line_->set_theme(theme_->GetTheme(THEME_STATUS_LINE));

  // Create scrolled window.
  if (!wxScrolledWindow::Create(parent, id)) {
    return false;
  }

  // Create line number area.
  if (!line_nr_area_->Create(this, wxID_ANY)) {
    return false;
  }
  line_nr_area_->SetBackgroundColour(style_->Get(Style::kNumber)->bg());

  // Create text area.
  if (!text_area_->Create(this, wxID_ANY)) {
    return false;
  }
  text_area_->SetBackgroundColour(theme_->GetColor(TEXT_BG));
  text_area_->SetCursor(wxCURSOR_IBEAM);

  // Set caret for text area.
  wxCaret* caret = new wxCaret(text_area_, kCaretWidth, -1);
  text_area_->SetCaret(caret);
  caret->Show();
  SetTargetWindow(text_area_);

  // Create status line.
  if (!status_line_->Create(this, wxID_ANY)) {
    return false;
  }

  UpdateLineNrWidth();
  // Update what are determined by text font (char width, line height, etc.).
  // TODO
  HandleTextFontChange();

  // Attach buffer listener when text window is actually created.
  buffer_->AttachListener(this);

  return true;
}

TextWindow::~TextWindow() {
  buffer_->DetachListener(this);
  delete buffer_; // FIXME

  if (wrap_helper_ != NULL) {
    delete wrap_helper_;
  }

  delete text_extent_;
}

void TextWindow::SetFocus() {
  if (text_area_ != NULL) {
    text_area_->SetFocus();
  }
}

//------------------------------------------------------------------------------

void TextWindow::SetTextFont(const wxFont& font) {
  text_area_->SetOwnFont(font);

  HandleTextFontChange();
  text_area_->Refresh();
}

void TextWindow::SetLineNrFont(const wxFont& font) {
  line_nr_area_->SetOwnFont(font);

  HandleLineNrFontChange();
  line_nr_area_->Refresh();
}

void TextWindow::SetStatusLineFont(const wxFont& font) {
  status_line_->SetFont(font);
  //LayoutAreas();
  status_line_->Refresh();
}

void TextWindow::SetStatusFields(const std::vector<StatusLine::FieldInfo>& field_infos) {
  status_line_->SetFields(field_infos);
}

//------------------------------------------------------------------------------
// Undo/redo

void TextWindow::Undo() {
  EditAction* edit_action = buffer_->Undo();
  if (edit_action != NULL) {
    DeleteRangeAction* delete_range_action = edit_action->AsDeleteRangeAction();
    if (delete_range_action != NULL) {
      SetSelection(delete_range_action->range(), delete_range_action->dir());
    } else {
      ClearSelection();
      UpdateCaretPoint(edit_action->point());
    }
  }
}

void TextWindow::Redo() {
  EditAction* edit_action = buffer_->Redo();
  if (edit_action != NULL) {
    if (edit_action->AsDeleteRangeAction() != NULL) {
      ClearSelection();
    }
    UpdateCaretPoint(edit_action->GetExecPoint());
  }
}

void TextWindow::Repeat() {
  ClearSelection();
  EditAction* edit_action = buffer_->Repeat(caret_point_);
  if (edit_action != NULL) {
    UpdateCaretPoint(edit_action->GetExecPoint());
  }
}

bool TextWindow::CanUndo() const {
  return buffer_->CanUndo();
}

bool TextWindow::CanRedo() const {
  return buffer_->CanRedo();
}

bool TextWindow::CanRepeat() const {
  return buffer_->CanRepeat();
}

void TextWindow::Exec(EditAction* edit_action) {
  EditAction* added_action = buffer_->AddEditAction(edit_action);
  UpdateCaretPoint(added_action->GetExecPoint());
}

//------------------------------------------------------------------------------

wxString TextWindow::buffer_file_name() const {
  return buffer_->file_name();
}

bool TextWindow::buffer_modified() const {
  return buffer_->modified();
}

// Overridden from BufferListener:

void TextWindow::OnBufferChange(ChangeType type, const ChangeData& data) {
  switch (type){
  case kLineAdded:
    HandleLineAdded(data);
    break;

  case kLineUpdated:
    HandleLineUpdated(data);
    break;

  case kLineDeleted:
    HandleLineDeleted(data);
    break;
  }

  if (buffer_modified_ != buffer_->modified()) {
    PostEvent(kEvtTextModifiedChange);
    buffer_modified_ = buffer_->modified();
  }
}

void TextWindow::OnBufferEncodingChange() {
  status_line_->NotifyChange(StatusLine::kField_Encoding);
  PostEvent(kEvtTextEncodingChange);
}

void TextWindow::OnBufferFileNameChange() {
  status_line_->NotifyChange(StatusLine::kField_FileName);
  PostEvent(kEvtTextFileNameChange);
}

void TextWindow::OnBufferSaved() {
  buffer_modified_ = false;
  PostEvent(kEvtTextModifiedChange);
}

void TextWindow::HandleLineUpdated(const ChangeData& data) {
  wxLogDebug(_T("Line updated: %d-%d"), data.first(), data.last());

  if (!wrapped_) {
    // Update text size and virtual size of areas, etc.
    // TODO: Optimize - When wrap is off, only need to recalculate virtual width.
    HandleTextChange();
    RefreshTextByLineRange(data, true);
  } else {
    int wrap_delta = 0;
    int client_width = text_area_->GetClientSize().GetWidth();
    for (Coord ln = data.first(); ln <= data.last(); ++ln) {
      wrap_delta += wrap_helper()->UpdateLineWrap(ln);
    }

    if (wrap_delta == 0) {
      // NO wrap change. Just refresh the lines in this range.
      RefreshTextByLineRange(data, true);
    } else {
      // Update text size and virtual size of areas, etc.
      // TODO: Optimize
      HandleTextChange();

      RefreshTextAfterLine(data.first(), false);
      RefreshLineNrAfterLine(data.first(), false);
    }
  }
}

void TextWindow::HandleLineAdded(const ChangeData& data) {
  wxLogDebug(_T("Line added: %d-%d"), data.first(), data.last());

  if (!wrapped_) {
    // Update text size and virtual size of areas, etc.
    /*bool area_resized = */HandleTextChange();

    // Note (2013-04-20):
    // HandleTextChange() might call LayoutAreas() which will trigger paint event by SetSize().
    // I really want to avoid the refresh here if LayoutAreas() was called. But the paint event
    // triggered by SetSize() cannot draw the new added line(s) and the lines after it
    // correctly (Tested in WinXP).
    // So always refresh here.

    //if (!area_resized) {
    RefreshTextAfterLine(data.first(), /*included=*/true);

    // Refresh the last "data.LineCount()" lines.
    // There'll be no paint event if they are out of the client rect.
    LineRange ln_refresh_range(buffer_->LineCount() - data.LineCount() + 1, buffer_->LineCount());
    RefreshLineNrByLineRange(ln_refresh_range, /*update=*/true);
    //}
  } else { // Wrapped
    int client_width = text_area_->GetClientSize().GetWidth();
    for (Coord ln = data.first(); ln <= data.last(); ++ln) {
      wrap_helper()->AddLineWrap(ln);
    }

    // Update text size and virtual size of areas, etc.
    HandleTextChange();

    //LineRange wrapped_line_range = wrap_helper()->Wrap(data);
    //text_area_->ScrollLineDown(wrapped_line_range.first(), wrapped_line_range.LineCount());
    RefreshTextAfterLine(data.first(), false);

    //LineRange client_line_range = GetClientLineRange();
    //LineRange refresh_line_range(data.first(), client_line_range.last());
    //RefreshLineNrByLineRange(refresh_line_range);
    RefreshLineNrAfterLine(data.first(), false);
  }
}

void TextWindow::HandleLineDeleted(const ChangeData& data) {
  wxLogDebug(_T("Line deleted: %d-%d"), data.first(), data.last());

  if (wrapped_) {
    // Note: In reverse!
    for (Coord ln = data.last(); ln >= data.first(); --ln) {
      wrap_helper()->RemoveLineWrap(ln);
    }
  }

  // Update text size and virtual size of areas, etc.
  HandleTextChange();

  // Refresh text area.
  if (data.first() == 1) { // First line is deleted.
    // Refresh the whole client.
    text_area_->Refresh();
  } else if (data.last() >= buffer_->LineCount()) { // Last line is deleted.
    // Now data.first() is invalid; data.first() - 1 must be valid.
    RefreshTextAfterLine(data.first() - 1, /*included=*/false);
  } else { // Middle lines are deleted.
    // Now data.first() is still valid.
    RefreshTextAfterLine(data.first(), /*included=*/true);
  }

  // Refresh line nr area.
  if (!wrapped_) {
    RefreshLineNrAfterLine(buffer_->LineCount(), /*included=*/true);
  } else {
    // For simplicity, just call Refresh().
    line_nr_area_->Refresh();
  }
}

void TextWindow::Wrap(bool wrapped) {
  if (wrapped == wrapped_) {
    return;
  }
  wrapped_ = wrapped;

  int wrap_delta = 0;
  bool wrap_changed = false;

  if (wrapped_) {
    wrap_changed = wrap_helper()->Wrap(wrap_delta);
  } else {
    if (wrap_helper_ != NULL) {
      wrap_changed = wrap_helper_->Unwrap(wrap_delta);
      wxDELETE(wrap_helper_);
    }
  }

  // Virtual height might change due to the wrap change.
  // The wrap-on virtual width is also different from the wrap-off virtual width.
  UpdateVirtualSize();

  if (wrap_changed) {
    text_area_->Refresh();
    if (wrap_delta != 0) {
      line_nr_area_->Refresh();
    }
  }

  // Caret position might change due to the wrap change.
  UpdateCaretPosition();
}

WrapHelper* TextWindow::wrap_helper() const {
  if (wrap_helper_ == NULL) {
    wrap_helper_ = new WrapHelper(buffer_, text_extent_);
    wrap_helper_->set_client_width(text_area_->GetClientSize().GetWidth());
  }
  return wrap_helper_;
}

void TextWindow::set_leader_key(Key key) {
  if (leader_key_ != key) {
    leader_key_ = key;
    status_line_->NotifyChange(StatusLine::kField_KeyStroke);
  }
}

//------------------------------------------------------------------------------

void TextWindow::OnSize(wxSizeEvent& evt) {
  if (m_targetWindow != this) {
    LayoutAreas();
    // NOTE (2013/04/14): Don't have to update virtual size!
  }
  evt.Skip();
}

void TextWindow::OnSetFocus(wxFocusEvent& evt) {
  if (text_area_ != NULL) {
    text_area_->SetFocus();
  }
}

//------------------------------------------------------------------------------
// Buffer change operations.

void TextWindow::InsertChar(wchar_t c) {
  Delete(kSelected, kWhole);

  InsertCharAction* insert_char_action = new InsertCharAction(buffer_, caret_point_, c);
  buffer_->AddInsertCharAction(insert_char_action);
  UpdateCaretPoint(insert_char_action->GetExecPoint());
}

void TextWindow::InsertString(const std::wstring& str) {
  Delete(kSelected, kWhole);
  Exec(new InsertStringAction(buffer_, caret_point_, str));
}

void TextWindow::InsertText(const std::wstring& text) {
  Delete(kSelected, kWhole);
  Exec(new InsertTextAction(buffer_, caret_point_, text));
}

void TextWindow::NewLineBelow() {
  ClearSelection();
  Exec(new NewLineBelowAction(buffer_, caret_point_));
}

void TextWindow::NewLineAbove() {
  ClearSelection();
  Exec(new NewLineAboveAction(buffer_, caret_point_));
}

void TextWindow::Move(TextUnit text_unit, SeekType seek_type) {
  TextPoint point = caret_point_;
  bool update_v = false;

  if (text_unit == kSelected) {
    if (!select_range_.Empty()) {
      if (seek_type == kBegin) {
        point = select_range_.first_point();
      } else if (seek_type == kEnd) {
        point = select_range_.last_point();
      }
    }
    update_v = true;
  } else {
    point = Seek(buffer_, caret_point_, text_unit, seek_type);
    update_v = !(text_unit == kLine && (seek_type == kPrev || seek_type == kNext));
  }

  // Clear selection even won't move actually.
  ClearSelection();

  if (point == caret_point_) {
    return;
  }

  if (!update_v) {
    point.x = std::min(v_caret_coord_x_, buffer_->LineLength(point.y));
  }

  UpdateCaretPoint(point, update_v);
}

void TextWindow::Delete(TextUnit text_unit, SeekType seek_type) {
  if (text_unit == kSelected) {
    if (!select_range_.Empty()) {
      DeleteRangeAction* action = new DeleteRangeAction(buffer_, select_range_, select_dir_, true);
      //ClearSelection(); // TODO
      select_range_.Reset();
      Exec(action);
    }
  } else if (text_unit == kLine && seek_type == kWhole) {
    Exec(new DeleteLineAction(buffer_, caret_point_));
  } else {
    if (Seek(buffer_, caret_point_, text_unit, seek_type) != caret_point_) {
      Exec(new DeleteAction(buffer_, caret_point_, text_unit, seek_type));
    }
  }
}

void TextWindow::Select(TextUnit text_unit, SeekType seek_type) {
  if (text_unit == kSelected && seek_type == kWhole) {
    // Clear current selection.
    ClearSelection();
    return;
  }

  if (text_unit == kBuffer && seek_type == kWhole) {
    // Select the whole buffer.
    Coord line_count = buffer_->LineCount();
    SetSelection(TextPoint(0, 1), TextPoint(buffer_->LineLength(line_count), line_count));
    return;
  }

  TextPoint point = Seek(buffer_, caret_point_, text_unit, seek_type);
  if (point != caret_point_) {
    ExtendSelection(point);
  }
}

void TextWindow::Change(const std::wstring& text) {
}

//------------------------------------------------------------------------------
// Scroll

void TextWindow::ScrollLinesDirectly(int lines) {
  int view_start_x = 0;
  int view_start_y = 0;
  GetViewStart(&view_start_x, &view_start_y);

  int y = view_start_y + lines;

  if (y != -1) {
    Scroll(-1, y);
  }
}

void TextWindow::GoTo(Coord ln) {
  if (ln < 1) {
    ln = 1;
  } else if (ln > buffer_->LineCount()) {
    ln = buffer_->LineCount();
  }

  LineRange line_range = GetClientLineRange();

  if (ln < line_range.first()) {
    ScrollLinesDirectly(ln - line_range.first() - GetPageSize() / 2);
  }
  if (ln > line_range.last()) {
    ScrollLinesDirectly(ln - line_range.last() + GetPageSize() / 2);
  }

  TextPoint point(std::min(v_caret_coord_x_, buffer_->LineLength(ln)), ln);
  UpdateCaretPoint(point, false, false);
}

//------------------------------------------------------------------------------

Coord TextWindow::GetPageSize() const {
  return (text_area_->GetClientSize().GetHeight() + line_height_) / line_height_;
}

//------------------------------------------------------------------------------

// Paint the text area.
void TextWindow::OnTextPaint(Renderer& renderer) {
  // Calculate lines to update from "update client rect".
  wxRect update_client_rect = text_area_->GetUpdateClientRect();
  LineRange line_range(LineFromScrolledY(update_client_rect.GetTop()),
                       LineFromScrolledY(update_client_rect.GetBottom()));
  if (line_range.IsEmpty()) {
    return; // No lines to update.
  }

  if (wrapped_) {
    // Convert to actual line range.
    // Note: After convert, the first line of the range might be only PARTIALLY visible.
    line_range = wrap_helper()->Unwrap(line_range);
  }

  //wxLogDebug(wxT("OnTextPaint, Update lines: %d-%d"), line_range.first(), line_range.last());

  const wxRect rect = text_area_->GetClientRect();
  int x = rect.GetLeft();

  renderer.SetPen(*wxTRANSPARENT_PEN);

  // Caret line highlight.
  // TODO: Wrap.
  if (line_range.Has(caret_point_.y)) {
    int hl = options_.line_highlight;
    if (hl == kHlText || hl == kHlBoth) {
      int y = rect.GetTop() + line_height_ * (caret_point_.y - 1);

      renderer.SetBrush(style_->Get(Style::kCaretLine)->bg());
      int hl_width = std::max(text_width_, rect.GetWidth());
      renderer.DrawRectangle(x, y, hl_width, char_height_);
    }
  }

  renderer.SetBrush(style_->Get(Style::kVisual)->bg());
  renderer.SetFont(text_area_->GetFont(), style_->Get(Style::kNormal)->fg());

  if (!wrapped_) {
    int y = rect.GetTop() + line_height_ * (line_range.first() - 1) + (line_height_ - char_height_) / 2;
    for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
      DrawTextLine(ln, renderer, x, y);
    }
  } else {
    // Note: The first line of the range might be only PARTIALLY visible.
    // That means we have to draw the invisible part (TODO).
    Coord wrapped_first_ln = wrap_helper()->WrapLineNr(line_range.first());
    int y = rect.GetTop() + line_height_ * (wrapped_first_ln - 1) + (line_height_ - char_height_) / 2;
    for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
      DrawWrappedTextLine(ln, renderer, x, y);
    }
  }
}

void TextWindow::DrawTextLine(Coord ln, Renderer& renderer, int x, int& y) {
  assert(!wrapped_);

  // If in select range, draw the select background.
  if (select_range_.HasLine(ln)) {
    CharRange char_range = select_range_.GetCharRange(ln);

    int x_begin = GetLineWidth(ln, 0, char_range.begin());
    int x_end = GetLineWidth(ln, 0, char_range.end());
    int w = x_end - x_begin;
    if (ln != select_range_.last_point().y && char_range.end() == kXCharEnd) {
      w += char_width_; // Extra char width for EOL.
    }

    renderer.DrawRectangle(x_begin, y, w, line_height_);
  }

  const TextLine& line = buffer_->GetLine(ln);
  RenderTextLine(renderer, line, x, y);

  y += line_height_;
}

void TextWindow::DrawWrappedTextLine(Coord ln, Renderer& renderer, int x, int& y) {
  assert(wrapped_);

  const TextLine& line = buffer_->GetLine(ln);

  const WrapInfo& wrap_info = wrap_helper()->GetWrapInfo(ln);
  std::vector<CharRange> sub_ranges;
  wrap_info.GetSubRanges(sub_ranges);

  // If in select range, draw the select background.
  if (select_range_.HasLine(ln)) {
    CharRange select_char_range = select_range_.GetCharRange(ln);

    for (size_t i = 0; i < sub_ranges.size(); ++i) {
      CharRange sub_select_char_range = sub_ranges[i].Intersect(select_char_range);
      if (sub_select_char_range.IsEmpty()) {
        continue; // No intersection with the select range.
      }

      int x_begin = GetLineWidth(line, sub_ranges[i].begin(), sub_select_char_range.begin());
      int x_end = GetLineWidth(line, sub_ranges[i].begin(), sub_select_char_range.end());
      int w = x_end - x_begin;
      if (ln != select_range_.last_point().y && sub_select_char_range.end() == kXCharEnd) {
        w += char_width_; // Extra char width for EOL.
      }

      renderer.DrawRectangle(x_begin, y + i * line_height_, w, line_height_);
    }
  }

  for (size_t i = 0; i < sub_ranges.size(); ++i) {
    renderer.DrawText(line.data(), x, y, sub_ranges[i].begin(), static_cast<size_t>(sub_ranges[i].CharCount()));
    y += line_height_;
  }
}

// Note: Cannot call Refresh() inside size event handler!
// Size change triggers refresh automatically when it's necessary (when the
// window gets larger - new client area has to be painted). If the window
// gets smaller, there'll be no refresh.
void TextWindow::OnTextSize(wxSizeEvent& evt) {
  //wxLogDebug(wxT("TextWindow::OnTextSize"));

  if (text_area_->GetClientSize().x < kUnreadyWindowSize) {
    return;
  }

  if (!wrapped_) {
    UpdateVirtualSize();
    // Don't call Refresh() here!
  } else {
    wrap_helper()->set_client_width(text_area_->GetClientSize().GetWidth());

    int wrap_delta = 0;
    bool wrap_changed = wrap_helper()->Wrap(wrap_delta);

    UpdateVirtualSize();

    if (wrap_changed) {
      // The text has to be repainted since the wrap changes.
      text_area_->Refresh();
      if (wrap_delta != 0) { // TODO
        line_nr_area_->Refresh();
      }

      // Caret position might change due to the wrap change.
      UpdateCaretPosition();
    }
  }
}

bool TextWindow::OnTextMouse(wxMouseEvent& evt) {
  bool handled = false;

  wxEventType evt_type = evt.GetEventType();
  if (evt_type == wxEVT_LEFT_DOWN) {
    handled = HandleTextLeftDown(evt);
  } else if (evt_type == wxEVT_MOTION) {
    HandleTextMouseMove(evt);
    handled = true;
  } else if (evt_type == wxEVT_LEFT_UP) {
    HandleTextMouseLeftUp(evt);
    handled = true;
  } else if (evt_type == wxEVT_MOUSEWHEEL) {
    handled = HandleTextMouseWheel(evt);
  }

  return handled;
}

bool TextWindow::HandleTextLeftDown(wxMouseEvent& evt) {
  text_area_->SetFocus();
  if (!text_area_->HasCapture()) {
    text_area_->CaptureMouse();
  }

  down_point_ = CalcCaretPoint(evt.GetPosition());

  if (evt.CmdDown()) {
    if (evt.AltDown() && evt.ShiftDown()) {
      HandleTextLeftDown_CtrlAltShift();
    } else if (evt.AltDown()) {
      HandleTextLeftDown_CtrlAlt();
    } else if (evt.ShiftDown()) {
      HandleTextLeftDown_CtrlShift();
    } else {
      HandleTextLeftDown_Ctrl();
    }
  } else {
    if (evt.AltDown() && evt.ShiftDown()) {
      HandleTextLeftDown_AltShift();
    } else if (evt.AltDown()) {
      HandleTextLeftDown_Alt();
    } else if (evt.ShiftDown()) {
      HandleTextLeftDown_Shift();
    } else {
      HandleTextLeftDown_NoAccel();
    }
  }

  return true;
}

void TextWindow::HandleTextLeftDown_NoAccel() {
  ClearSelection();
  UpdateCaretPoint(down_point_);
}

void TextWindow::HandleTextLeftDown_Ctrl() {
  if (select_range_.Contain(down_point_)) {
    // Click on selected text, select the bracket pair range.
    TextRange bracket_pair_range;
    if (buffer_->IsBracketPairInnerRange(select_range_)) {
      // If it's already inner range, increase to outer range.
      bracket_pair_range = select_range_;
      buffer_->IncreaseRange(bracket_pair_range);
      SetSelection(bracket_pair_range, kForward);
    } else { // Get inner range.
      if (buffer_->GetBracketPairInnerRange(select_range_, bracket_pair_range)) {
        SetSelection(bracket_pair_range, kForward);
      }
    }
  } else { // Click on unselected text.
    ClearSelection();
    // Select the word under the caret.
    TextRange word_range(Seek(buffer_, down_point_, kWord, kBegin),
                         Seek(buffer_, down_point_, kWord, kEnd));
    SetSelection(word_range, kForward);
  }

  down_modifiers_ = wxMOD_CONTROL;
}

void TextWindow::HandleTextLeftDown_CtrlAlt() {
  down_modifiers_ = wxMOD_CONTROL | wxMOD_ALT;
}

void TextWindow::HandleTextLeftDown_CtrlShift() {
  down_modifiers_ = wxMOD_CONTROL | wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_CtrlAltShift() {
  down_modifiers_ = wxMOD_CONTROL | wxMOD_ALT | wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_Alt() {
  ClearSelection();
  UpdateCaretPoint(down_point_);

  down_modifiers_ = wxMOD_ALT;
}

void TextWindow::HandleTextLeftDown_Shift() {
  ClearSelection();
  UpdateCaretPoint(down_point_);

  down_modifiers_ = wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_AltShift() {
  ClearSelection();
  UpdateCaretPoint(down_point_);

  down_modifiers_ = wxMOD_ALT | wxMOD_SHIFT;
}

void TextWindow::HandleTextMouseMove(wxMouseEvent& evt) {
  if (!text_area_->HasCapture()) {
    return;
  }
  if (!evt.LeftIsDown()) {
    return;
  }

  dragged_ = true;

  move_position_ = evt.GetPosition();
  wxRect client_rect = text_area_->GetClientRect();

  if (move_position_.y > client_rect.GetBottom()) {
    scroll_dir_ = kScrollDown;
    StartScrollTimer();
  } else if (move_position_.y < client_rect.GetTop()) {
    scroll_dir_ = kScrollUp;
    StartScrollTimer();
  } else {
    StopScrollTimer();
  }

  SelectByDragging();
}

void TextWindow::SelectByDragging() {
  TextPoint move_point = CalcCaretPoint(move_position_);

  if ((down_modifiers_ & wxMOD_CONTROL) == wxMOD_CONTROL) {
    // Select text by word.
    bool fw = move_point >= down_point_;
    TextPoint from = Seek(buffer_, down_point_, kWord, fw ? kBegin : kEnd);
    TextPoint to = Seek(buffer_, move_point, kWord, fw ? kEnd : kBegin);
    SetSelection(from, to);
  } else {
    // Select text by char.
    if (move_point != caret_point_) {
      ExtendSelection(move_point);
    }
  }
}

void TextWindow::StartScrollTimer() {
  // Calculate the speed rate according to distance between
  wxRect client_rect = text_area_->GetClientRect();
  int speed_rate = 0;
  if (scroll_dir_ == kScrollDown) {
    speed_rate = (move_position_.y - client_rect.GetBottom()) / 3;
  } else {
    speed_rate = (client_rect.GetTop() - move_position_.y) / 3;
  }
  if (speed_rate < 1) {
    speed_rate = 1;
  }

  int interval = 200 / speed_rate;
  if (interval < 10) {
    interval = 10;
  }

  if (scroll_timer_ == NULL) {
    scroll_timer_ = new wxTimer(this, wxID_ANY);
    Connect(scroll_timer_->GetId(), wxEVT_TIMER, wxTimerEventHandler(TextWindow::OnScrollTimer));
  }

  if (scroll_timer_->IsRunning()) {
    if (interval != scroll_timer_->GetInterval()) {
      // Stop to change the interval.
      scroll_timer_->Stop();
    } else {
      return;
    }
  }

  scroll_timer_->Start(interval, wxTIMER_CONTINUOUS);

  if (scroll_dir_ == kScrollDown) {
    ScrollLines(1);
  } else if (scroll_dir_ == kScrollUp) {
    ScrollLines(-1);
  }
}

void TextWindow::StopScrollTimer() {
  if (scroll_timer_ != NULL) {
    Disconnect(wxEVT_TIMER, scroll_timer_->GetId());
    if (scroll_timer_->IsRunning()) {
      scroll_timer_->Stop();
    }
    delete scroll_timer_;
    scroll_timer_ = NULL;
  }
}

void TextWindow::OnScrollTimer(wxTimerEvent& evt) {
  if (scroll_dir_ == kScrollDown) {
    ScrollLines(1);
  } else if (scroll_dir_ == kScrollUp) {
    ScrollLines(-1);
  }
  SelectByDragging();
}

void TextWindow::HandleTextMouseLeftUp(wxMouseEvent& evt) {
  if (text_area_->HasCapture()) {
    text_area_->ReleaseMouse();
  }

  StopScrollTimer();

  if (!dragged_) {
    // TODO
    if (down_modifiers_ == (wxMOD_CONTROL | wxMOD_ALT)) {
      //SelectToLineBegin();
    } else if (down_modifiers_ == wxMOD_ALT) {
      //SelectToLineEnd();
    }
  }

  // Clear flags.
  down_modifiers_ = 0;
  dragged_ = false;
}

bool TextWindow::HandleTextMouseWheel(wxMouseEvent& evt) {
  if (!evt.ControlDown()) {
    // Return false to let the event skip for the default handling: scroll vertically.
    return false;
  }

  // CMD is down, scroll horizontally.
  wxRect client_rect = text_area_->GetClientRect();

  int unscrolled_right = 0;
  CalcUnscrolledPosition(client_rect.GetRight(), 0, &unscrolled_right, NULL);

  int view_start_x = 0;
  GetViewStart(&view_start_x, NULL);

  int x = -1;

  if (evt.GetWheelRotation() > 0) {
    if (view_start_x > 0) {
      x = view_start_x - evt.GetLinesPerAction();
      if (x < 0) {
        x = 0;
      }
    }
  } else if (evt.GetWheelRotation() < 0) {
    x = view_start_x + evt.GetLinesPerAction();
  }

  if (x != -1) {
    Scroll(x, -1);
  }

  return true;
}

void TextWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Stop select-by-dragging.
  StopScrollTimer();
}

// About the return value:
// - true: the event won't be skipped
// - false: the event will be skipped
// If the key down event is not skipped, no char event.
bool TextWindow::OnTextKeyDown(wxKeyEvent& evt) {
  const int key_code = evt.GetKeyCode();

  // Key code is 0 when, e.g., you type Chinese.
  if (key_code == WXK_NONE) {
    return false; // Don't return true, or there might be characters lost.
  }

  if (key_code == WXK_CONTROL || key_code == WXK_SHIFT || key_code == WXK_ALT) {
    // Modifiers are used to modify other keys, don't handle them.
    // Don't clear leader key if any!
    return false;
  }

  if (key_code == WXK_ESCAPE) {
    if (!leader_key_.IsEmpty()) {
      leader_key_.Reset();
      status_line_->NotifyChange(StatusLine::kField_KeyStroke);
      return true;
    }
    // Note: ESC should also be bound to clear the current selection.
  }

  const int modifiers = evt.GetModifiers() ;

  if (leader_key_.IsEmpty() && modifiers != 0) { // Leader key always has modifiers.
    if (binding_->IsLeaderKey(key_code, modifiers)) {
      leader_key_.Set(key_code, modifiers);
      status_line_->NotifyChange(StatusLine::kField_KeyStroke);
      return true;
    }
  }

  if (key_code == WXK_TAB && modifiers == 0 && leader_key_.IsEmpty()) {
    // Input tab (expand or not).
    if (options_.expand_tab) {
      int spaces = options_.tab_stop - (caret_point_.x % options_.tab_stop);
      InsertString(std::wstring(spaces, kSpaceChar));
    } else {
      InsertChar(kTabChar);
    }
    return true;
  }

  // Avoid matching for single character key.
  if (leader_key_.IsEmpty() && modifiers == 0) {
    if (key_code >= 33 && key_code <= 126 || // Standard ASCII characters.
        key_code >= 128 && key_code <= 255) { // ASCII extended characters.
      return false;
    }
  }

  // Now match the binding.
  bool handled = false;

  Mode mode = select_range_.Empty() ? kNormalMode : kVisualMode;

  Key key_stroke(key_code, modifiers);
  if (!leader_key_.IsEmpty()) {
    key_stroke.set_leader(leader_key_);
  }

  TextFunc text_func = binding_->Find(key_stroke, mode);
  if (text_func != NULL) {
    text_func(this);
    handled = true;
  }

  if (!leader_key_.IsEmpty()) {
    leader_key_.Reset();
    status_line_->NotifyChange(StatusLine::kField_KeyStroke);
    handled = true; // Avoid char event.
  }

  if (!text_func) {
    // No binding matched. (TODO: Show in status line.)
    wxLogDebug(key_stroke.ToString() + wxT(" is not a command!"));
  }

  if (!handled) {
    if (key_code == WXK_INSERT) {
      // INSERT key is not bound to any edit action.
      // And Jil has NO insert mode!
      handled = true;
    }
  }

  return handled;
}

void TextWindow::OnTextChar(wxKeyEvent& evt) {
  if (evt.AltDown() || evt.CmdDown()) {
    evt.Skip();
    return;
  }

  wchar_t c = evt.GetUnicodeKey();
  if (c < 0x20) { // CONTROL
    // TODO
  } else {
    InsertChar(c);
  }

  //wxLogDebug(_T("Input char: %d"), (int)evt.GetUnicodeKey());
}

//------------------------------------------------------------------------------

// Paint the line number area.
void TextWindow::OnLineNrPaint(wxDC& dc) {
  // Calculate lines to update from "update client rect".
  wxRect update_client_rect = line_nr_area_->GetUpdateClientRect();
  LineRange line_range(LineFromScrolledY(update_client_rect.GetTop()),
                       LineFromScrolledY(update_client_rect.GetBottom()));
  if (line_range.IsEmpty()) {
    return; // No lines to update.
  }

  if (wrapped_) {
    line_range = wrap_helper()->Unwrap(line_range);
  }

  //wxLogDebug(wxT("OnLineNrPaint, Update lines: %d-%d"), line_range.first(), line_range.last());

  const wxRect client_rect = line_nr_area_->GetClientRect();
  int x = client_rect.GetLeft() + GetUnscrolledX(0);

  dc.SetPen(*wxTRANSPARENT_PEN);

  // Caret line highlight.
  if (line_range.Has(caret_point_.y)) {
    if (options_.line_highlight == kHlNumber || options_.line_highlight == kHlBoth) {
      Coord first_ln = caret_point_.y;
      if (wrapped_) {
        first_ln = wrap_helper()->WrapLineNr(first_ln);
      }
      int y = client_rect.GetTop() + line_height_ * (first_ln - 1);
      int w = client_rect.GetWidth() - 3; // TODO: 3
      int h = char_height_;
      if (wrapped_) {
        h = char_height_ * wrap_helper()->SubLineCount(caret_point_.y);
      }

      dc.SetBrush(style_->Get(Style::kCaretLine)->bg());
      dc.DrawRectangle(x, y, w, h);
    }
  }

  dc.SetFont(line_nr_area_->GetFont());
  dc.SetTextForeground(style_->Get(Style::kNumber)->fg());

  // Draw right aligned line numbers.
  Coord first_ln = line_range.first();
  if (wrapped_) {
    first_ln = wrap_helper()->WrapLineNr(first_ln);
  }
  int y = client_rect.GetTop() + line_height_ * (first_ln - 1);
  wxRect rect(x, y, client_rect.GetWidth() - kLineNrPaddingRight, line_height_);

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    dc.DrawLabel(wxString::Format(_T("%d"), ln), rect, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    if (!wrapped_) {
      rect.SetY(rect.GetY() + line_height_);
    } else {
      rect.SetY(rect.GetY() + line_height_ * wrap_helper()->SubLineCount(ln));
    }
  }
}

void TextWindow::OnLineNrSize(wxSizeEvent& evt) {
  line_nr_area_->Refresh();
}

static size_t CountCharAfter(const std::wstring& line_data, size_t i, wchar_t c) {
  size_t j = i + 1;
  for (; j < line_data.size(); ++j) {
    if (line_data[j] != c) {
      break;
    }
  }
  return j - i - 1;
}

void TextWindow::RenderTextLine(Renderer& renderer, const TextLine& line, int x, int y) {
  const std::wstring& line_data = line.data();
  if (line_data.empty()) {
    return;
  }

  const int space_w = text_extent_->GetWidth(wxT(" "));

  int _x = x;
  int _y = y;

  Coord chars = 0;
  size_t p = 0;
  for (size_t i = 0; i < line_data.size(); ++i) {
    if (line_data[i] != kSpaceChar && line_data[i] != kTabChar) {
      ++chars;
      continue;
    }

    // Render the line piece before this space or tab.
    if (i > p) {
      int piece_w = 0;
      renderer.DrawText(line_data.substr(p, i - p), _x, _y, &piece_w);
      _x += piece_w;
    }

    if (line_data[i] == kSpaceChar) {
      size_t spaces = CountCharAfter(line_data, i, kSpaceChar) + 1;
      if (options_.show_space) {
        renderer.SetPen(*wxRED_PEN);
        renderer.DrawWhiteSpaces(_x, _y, spaces);
      }
      _x += space_w * spaces;
      chars += spaces;
      i += spaces - 1;
      p = i + 1;

    } else if (line_data[i] == kTabChar) {
      int tab_spaces = options_.tab_stop - (chars % options_.tab_stop);
      chars += tab_spaces;

      size_t tabs = CountCharAfter(line_data, i, kTabChar) + 1;

      // TODO
      int tab_w = space_w * tab_spaces;
      if (options_.show_space) {
        renderer.SetPen(*wxRED_PEN);
        renderer.DrawTabs(_x, _y, tab_w, char_height_);
      }

      _x += tab_w;
      p = i + 1;
    }
  }

  // Render the last line piece if any.
  if (p < line_data.size()) {
    std::wstring last_piece = line_data.substr(p, line_data.size() - p);
    renderer.DrawText(last_piece, _x, _y);
  }
}

Coord TextWindow::LineFromScrolledY(int scrolled_y) const {
  Coord line = (GetUnscrolledY(scrolled_y) + line_height_) / line_height_;

  Coord line_count = buffer_->LineCount();
  if (wrapped_) {
    line_count = wrap_helper()->SubLineCount();
  }

  if (line > line_count) {
    line = line_count;
  }
  return line;
}

TextPoint TextWindow::CalcCaretPoint(const wxPoint& position) {
  // Adjust the position if it's out of the text client area.
  // This happens when drag mouse to select text.
  wxPoint adjusted_position = position;
  wxRect client_rect = text_area_->GetClientRect();
  if (adjusted_position.y > client_rect.GetBottom()) {
    adjusted_position.y = client_rect.GetBottom();
  } else if (adjusted_position.y < client_rect.GetTop()) {
    adjusted_position.y = client_rect.GetTop();
  }

  int unscrolled_x = 0;
  int unscrolled_y = 0;
  CalcUnscrolledPosition(adjusted_position.x, adjusted_position.y, &unscrolled_x, &unscrolled_y);

  TextPoint caret_point;

  caret_point.y = unscrolled_y / line_height_;
  if (unscrolled_y % line_height_ > 0) {
    ++caret_point.y;
  }

  if (!wrapped_) {
    if (caret_point.y < 1) {
      caret_point.y = 1;
    } else if (caret_point.y > buffer_->LineCount()) {
      caret_point.y = buffer_->LineCount();
    }

    caret_point.x = GetCharIndex(caret_point.y, unscrolled_x);

  } else { // wrapped
    int wrapped_ln = caret_point.y;

    if (wrapped_ln < 1) {
      wrapped_ln = 1;
    } else {
      Coord sub_line_count = wrap_helper()->SubLineCount();
      if (wrapped_ln > sub_line_count) {
        wrapped_ln = sub_line_count;
      }
    }

    int sub_ln = 0;
    int ln = wrap_helper()->UnwrapLineNr(wrapped_ln, sub_ln);

    caret_point.y = ln;
    caret_point.x = GetWrappedCharIndex(caret_point.y, sub_ln, unscrolled_x);
  }

  return caret_point;
}

//wxRect TextWindow::GetLineTextClientRect(Coord ln) const {
//  assert(ln > 0);
//  int scrolled_x = 0;
//  int scrolled_y = 0;
//  CalcScrolledPosition(0, line_height_ * (ln - 1), &scrolled_x, &scrolled_y);
//  return wxRect(scrolled_x, scrolled_y, text_area_->GetClientRect().GetRight() - scrolled_x, line_height_);
//}

wxRect TextWindow::ClientRectFromLineRange(wxWindow* area, const LineRange& line_range) const {
  //wxLogDebug("TextWindow::ClientRectFromLineRange: %d-%d/%d", line_range.first(), line_range.last(), buffer_->LineCount());
  LineRange line_range_copy = line_range;
  if (wrapped_) {
    line_range_copy = wrap_helper()->Wrap(line_range_copy);
  }

  int scrolled_y = GetScrolledY(line_height_ * (line_range_copy.first() - 1));

  const wxRect client_rect = area->GetClientRect();
  return wxRect(0, scrolled_y, client_rect.GetWidth(), line_height_ * line_range_copy.LineCount());
}

wxRect TextWindow::ClientRectAfterLine(wxWindow* area, Coord ln, bool included) const {
  //wxLogDebug("TextWindow::ClientRectAfterLine");

  Coord v_ln = ln;
  if (wrapped_) {
    v_ln = wrap_helper()->WrapLineNr(ln);
  }

  if (!included) {
    if (!wrapped_) {
      --v_ln;
    } else {
      v_ln -= wrap_helper()->SubLineCount(ln);
    }
  }

  int y = GetScrolledY(line_height_ * (v_ln - 1));

  const wxRect rect = area->GetClientRect();

  if (y <= rect.GetTop()) {
    return rect;
  } else if (y > rect.GetBottom()) {
    return wxRect(); // Empty
  } else {
    return wxRect(rect.GetLeft(), y, rect.GetWidth(), rect.GetHeight() - y);
  }
}

//wxRect TextWindow::GetCharTextClientRect(Coord caret_y_line, Coord caret_x_char) const {
//  assert(caret_y_line > 0 && caret_x_char >= 0);
//  int virtual_text_client_x = caret_x_char * char_width_;
//  int virtual_text_client_y = line_height_ * (caret_y_line - 1);
//  int text_client_x = 0;
//  int text_client_y = 0;
//  CalcScrolledPosition(virtual_text_client_x,
//                       virtual_text_client_y,
//                       &text_client_x,
//                       &text_client_y);
//  return wxRect(text_client_x, text_client_y, char_width_, line_height_);
//}

// Note: @ln might be invalid.
// E.g., called by UpdateCaretPoint() to erase the previous caret line which has been deleted.
void TextWindow::RefreshTextByLine(Coord ln, bool update) {
  //wxLogDebug("TextWindow::RefreshTextByLine: %d", ln);
  if (ln <= buffer_->LineCount()) {
    RefreshTextByLineRange(LineRange(ln), update);
  }
}

// Note: @ln might be invalid.
// E.g., called by UpdateCaretPoint() to erase the previous caret line which has been deleted.
void TextWindow::RefreshLineNrByLine(Coord ln, bool update) {
  //wxLogDebug("TextWindow::RefreshLineNrByLine: %d", ln);
  if (ln <= buffer_->LineCount()) {
    RefreshLineNrByLineRange(LineRange(ln), update);
  }
}

void TextWindow::RefreshTextAfterLine(Coord ln, bool included, bool update) {
  //wxLogDebug("TextWindow::RefreshTextAfterLine");
  wxRect rect = ClientRectAfterLine(text_area_, ln, included);
  if (!rect.IsEmpty()) {
    text_area_->RefreshRect(rect);
    if (update) {
      text_area_->Update();
    }
  }
}

void TextWindow::RefreshLineNrAfterLine(Coord ln, bool included, bool update) {
  //wxLogDebug("TextWindow::RefreshLineNrAfterLine");
  wxRect rect = ClientRectAfterLine(line_nr_area_, ln, included);
  if (!rect.IsEmpty()) {
    line_nr_area_->RefreshRect(rect);
    if (update) {
      line_nr_area_->Update();
    }
  }
}

void TextWindow::RefreshTextByLineRange(const LineRange& line_range, bool update) {
  //wxLogDebug("TextWindow::RefreshTextByLineRange: %d-%d/%d", line_range.first(), line_range.last(), buffer_->LineCount());
  wxRect refresh_rect = ClientRectFromLineRange(text_area_, line_range);
  //wxLogDebug("Refresh text rect: %d,%d,%d,%d.",
  //           refresh_rect.GetLeft(),
  //           refresh_rect.GetTop(),
  //           refresh_rect.GetWidth(),
  //           refresh_rect.GetHeight());
  text_area_->RefreshRect(refresh_rect);
  if (update) {
    text_area_->Update();
  }
}

void TextWindow::RefreshLineNrByLineRange(const LineRange& line_range, bool update) {
  //wxLogDebug("TextWindow::RefreshLineNrByLineRange: %d-%d", line_range.first(), line_range.last());
  wxRect rect = ClientRectFromLineRange(line_nr_area_, line_range);
  //wxLogDebug("RefreshLineNrByLineRange rect: %d,%d,%d,%d.",
  //           rect.GetLeft(),
  //           rect.GetTop(),
  //           rect.GetWidth(),
  //           rect.GetHeight());
  line_nr_area_->RefreshRect(rect);
  if (update) {
    line_nr_area_->Update();
  }
}

void TextWindow::HandleTextFontChange() {
  // Update the font of text extent.
  text_extent_->SetFont(text_area_->GetFont());

  UpdateCharSize();
  SetScrollbars(char_width_, line_height_, 1, 1);

  UpdateTextSize();
  UpdateVirtualSize();

  // Update caret height.
  text_area_->GetCaret()->SetSize(kCaretWidth, line_height_);
}

void TextWindow::HandleLineNrFontChange() {
  int old_line_nr_width = line_nr_width_;
  UpdateLineNrWidth();

  // If the line number width changes, layout the areas.
  if (old_line_nr_width != line_nr_width_) {
    LayoutAreas();
  }
}

bool TextWindow::HandleTextChange() {
  UpdateTextSize();

  // Update the width of line number area since the number of lines might change.
  int old_line_nr_width = line_nr_width_;
  UpdateLineNrWidth();

  bool resized = false;

  // If the line number width changes, layout the areas.
  if (old_line_nr_width != line_nr_width_) {
    LayoutAreas();
    resized = true;
  }

  // If resized (LayoutAreas() is called), there will be size event triggered,
  // and in the size event handler, UpdateVirtualSize() will be called.
  // See OnTextSize().
  if (!resized) {
    UpdateVirtualSize();
  }

  return resized;
}

// TODO: external leading
void TextWindow::UpdateCharSize() {
  int external_leading = 0;
  text_extent_->GetExtent(_T("W"), &char_width_, &char_height_, &external_leading);
  //wxLogDebug(_T("external_leading: %d"), external_leading);
  line_height_ = char_height_ + external_leading + kLinePaddingY;
}

void TextWindow::UpdateTextSize() {
  if (!wrapped_) {
    text_width_ = char_width_ * buffer_->GetMaxLineLength();
    text_height_ = line_height_ * buffer_->LineCount();
  } else {
    text_width_ = -1;
    // TODO: Optimize (WrappedLineCount() is in linear time).
    text_height_ = line_height_ * wrap_helper()->SubLineCount();
  }
}

void TextWindow::UpdateLineNrWidth() {
  wxString line_nr_str = wxString::Format(L"%d", buffer_->LineCount());
  line_nr_width_ = text_extent_->GetWidth(line_nr_str.wc_str()) + kLineNrPadding;
}

void TextWindow::UpdateVirtualSize() {
  wxLogDebug("TextWindow::UpdateVirtualSize");

  Coord line_count = !wrapped_ ? buffer_->LineCount() : wrap_helper()->SubLineCount();

  // - 2 to keep the last line visible when scroll to the end.
  // Why doesn't "- 1" work?
  int vh = line_height_ * (line_count + GetPageSize() - 2);

  if (!wrapped_) {
    // - char_width_ to keep the last char visible when scroll to the end.
    int vw = text_width_ + text_area_->GetClientSize().GetWidth() - char_width_;
    text_area_->SetVirtualSize(vw, vh);
  } else {
    text_area_->SetVirtualSize(-1, vh);
  }

  line_nr_area_->SetVirtualSize(-1, vh);

  AdjustScrollbars();
}

void TextWindow::LayoutAreas() {
  const wxRect client_rect = GetClientRect();

  const int kStatusPaddingY = 2;
  int status_height = line_height_ + kStatusPaddingY;

  int text_area_height = client_rect.GetHeight() -
                         kMarginTop -
                         kMarginBottom -
                         kTextMarginBottom -
                         status_height;

  line_nr_area_->SetSize(0, kMarginTop, line_nr_width_, text_area_height);

  text_area_->SetSize(line_nr_width_ + kTextMarginLeft,
                      kMarginTop,
                      client_rect.GetWidth() - kTextMarginLeft - line_nr_width_,
                      text_area_height);

  status_line_->SetSize(0, client_rect.GetBottom() - status_height + 1,
                        client_rect.GetWidth(), status_height);
}

void TextWindow::UpdateCaretPoint(const TextPoint& point, bool update_v, bool scroll) {
  if (point == caret_point_) {
    return;
  }

  // Refresh the highlight of the caret line if necessary.
  int hl = options_.line_highlight;
  if (caret_point_.y != point.y && hl != kHlNone) {
    Coord prev_y = caret_point_.y;
    caret_point_ = point;

    if (hl == kHlText || hl == kHlBoth) {
      RefreshTextByLine(prev_y); // Erase
      RefreshTextByLine(caret_point_.y);
    } else if (hl == kHlNumber || hl == kHlBoth) {
      RefreshLineNrByLine(prev_y); // Erase
      RefreshLineNrByLine(caret_point_.y);
    }
  } else {
    caret_point_ = point;
  }

  if (update_v) {
    v_caret_coord_x_ = point.x;
  }

  UpdateCaretPosition(scroll);

  // Notify status line to update caret position.
  status_line_->NotifyChange(StatusLine::kField_Caret);
}

void TextWindow::UpdateCaretPosition(bool scroll) {
  if (!caret_point_.Valid()) {
    return;
  }

  wxLogDebug("TextWindow::UpdateCaretPosition");

  size_t x_off = 0;
  int y = 0;

  if (!wrapped_) {
    y = caret_point_.y;
  } else {
    Coord sub_ln = wrap_helper()->GetWrapInfo(caret_point_.y).GetSubLineNr(caret_point_.x, &x_off);
    y = wrap_helper()->WrapLineNr(caret_point_.y) + sub_ln - 1;
  }

  int unscrolled_x = GetLineWidth(caret_point_.y, x_off, caret_point_.x);
  int unscrolled_y = (y - 1) * line_height_;
  wxPoint p;
  CalcScrolledPosition(unscrolled_x, unscrolled_y, &p.x, &p.y);
  text_area_->GetCaret()->Move(p);

  if (scroll) {
    ScrollToCaret();
  }
}

void TextWindow::ScrollToCaret() {
  if (!caret_point_.Valid()) {
    return;
  }

  wxLogDebug("TextWindow::ScrollToCaret");

  int view_start_x = 0;
  int view_start_y = 0;
  GetViewStart(&view_start_x, &view_start_y);

  int y = -1;

  Coord caret_y = caret_point_.y;
  if (wrapped_) {
    caret_y = wrap_helper()->WrapLineNr(caret_y) + wrap_helper()->SubLineCount(caret_y) - 1;
  }

  // - 1 because line nr is 0-based.
  if (caret_y - 1 < view_start_y) {
    y = caret_y - 1;
  } else {
    int unscrolled_bottom = GetUnscrolledY(text_area_->GetClientRect().GetBottom());
    int line_end = (unscrolled_bottom + line_height_ / 2) / line_height_;
    if (caret_y > line_end) {
      y = view_start_y + (caret_y - line_end);
    }
  }

  int x = -1;

  // If wrap is on, no horizontal scroll.
  if (!wrapped_) {
    const int kScrollRateX = 3;  // 3 units per scroll.
    if (caret_point_.x <= view_start_x) {
      x = caret_point_.x - kScrollRateX;
      if (x < 0) {
        x = 0;
      }
    } else {
      int unscrolled_right = GetUnscrolledX(text_area_->GetClientRect().GetRight());
      int char_end = (unscrolled_right + char_width_ / 2) / char_width_;
      if (caret_point_.x >= char_end) {
        // 3 units per scroll.
        x = view_start_x + (caret_point_.x - char_end) + kScrollRateX;
      }
    }
  }

  if (x != -1 || y != -1) {
    Scroll(x, y);
  }
}

void TextWindow::SetSelection(const TextRange& range, TextDir select_dir) {
  TextRange old_range = select_range_;
  select_range_ = range;

  select_dir_ = select_dir;

  TextPoint point_to = select_dir == kForward ? select_range_.last_point() : select_range_.first_point();
  if (caret_point_ != point_to) {
    UpdateCaretPoint(point_to);
  }

  std::vector<Coord> lines_to_refresh;
  DiffTextRangeByLine(old_range, select_range_, lines_to_refresh);

  // Now refresh the lines to refresh.
  // TODO: Optimize
  foreach (Coord ln, lines_to_refresh) {
    RefreshTextByLine(ln, true);
    wxLogDebug(_T("Select refresh line: %d"), ln);
  }
}

void TextWindow::SetSelection(const TextPoint& point_from, const TextPoint& point_to) {
  if (point_from <= point_to) {
    SetSelection(TextRange(point_from, point_to), kForward);
  } else {
    SetSelection(TextRange(point_to, point_from), kBackward);
  }
}

// TODO Refresh optimization.
void TextWindow::ClearSelection() {
  if (select_range_.Empty()) {
    return;
  }

  TextRange old_range = select_range_;
  select_range_.Reset();
  std::vector<Coord> lines_to_refresh;
  DiffTextRangeByLine(old_range, select_range_, lines_to_refresh);
  foreach (Coord ln, lines_to_refresh) {
    RefreshTextByLine(ln, true);
  }
}

void TextWindow::ExtendSelection(const TextPoint& point_to) {
  if (select_range_.Empty()) {
    SetSelection(caret_point_, point_to);
  } else {
    if (select_dir_ == kForward) {
      SetSelection(select_range_.first_point(), point_to);
    } else {
      SetSelection(select_range_.last_point(), point_to);
    }
  }
}

int TextWindow::GetLineWidth(const TextLine& line, size_t off1, size_t off2) const {
  std::wstring line_data = line.data().substr(off1, off2 - off1);
  if (line_data.empty()) {
    return 0;
  }
  TabbedLineFast(options_.tab_stop, line_data);
  return text_extent_->GetWidth(line_data);
}

int TextWindow::GetLineWidth(Coord ln, size_t off1, size_t off2) const {
  return GetLineWidth(buffer_->GetLine(ln), off1, off2);
}

int TextWindow::GetCharIndex(Coord ln, int text_client_x) const {
  const std::wstring& line = buffer_->GetLine(ln).data();
  if (line.empty()) {
    return 0;
  }

  int index = IndexChar(*text_extent_, options_.tab_stop, line, text_client_x);
  if (index == -1) {
    index = line.length();
  }
  //wxLogDebug(_T("GetCharIndex: %d"), index);
  return index;
}

// TODO
int TextWindow::GetWrappedCharIndex(Coord ln, int wrap_sub_ln, int text_client_x) const {
  assert(wrap_sub_ln >= 1);

  if (wrap_sub_ln == 1) {
    return GetCharIndex(ln, text_client_x); // TODO
  } else { // wrap_sub_line > 1
    size_t offset_index = wrap_sub_ln - 2;

    const WrapInfo& wrap_info = wrap_helper()->GetWrapInfo(ln);

    size_t offset = wrap_info.offsets()[offset_index];

    std::wstring line = buffer_->GetLine(ln).data().substr(offset);

    int index = IndexChar(*text_extent_, options_.tab_stop, line, text_client_x);
    if (index == -1) { // TODO
      index = line.length();
    }
    return offset + index;
  }
}

int TextWindow::GetUnscrolledX(int scrolled_x) const {
  int unscrolled_x = 0;
  CalcUnscrolledPosition(scrolled_x, 0, &unscrolled_x, NULL);
  return unscrolled_x;
}

int TextWindow::GetUnscrolledY(int scrolled_y) const {
  int unscrolled_y = 0;
  CalcUnscrolledPosition(0, scrolled_y, NULL, &unscrolled_y);
  return unscrolled_y;
}

int TextWindow::GetScrolledX(int unscrolled_x) const {
  int scrolled_x = 0;
  CalcScrolledPosition(unscrolled_x, 0, &scrolled_x, NULL);
  return scrolled_x;
}

int TextWindow::GetScrolledY(int unscrolled_y) const {
  int scrolled_y = 0;
  CalcScrolledPosition(0, unscrolled_y, NULL, &scrolled_y);
  return scrolled_y;
}

LineRange TextWindow::GetClientLineRange() const {
  LineRange line_range;
  int unscrolled_y = 0;
  CalcUnscrolledPosition(0, 0, NULL, &unscrolled_y);
  line_range.set_first((unscrolled_y + line_height_) / line_height_);
  line_range.set_last(line_range.first() + GetPageSize() - 1);
  return line_range;
}

//------------------------------------------------------------------------------

FileType* TextWindow::file_type() const {
  return buffer_->file_type();
}

void TextWindow::PostEvent(wxEventType evt_type) {
  wxCommandEvent evt(evt_type, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

////////////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(kEvtTextEncodingChange);
DEFINE_EVENT_TYPE(kEvtTextFileNameChange);
DEFINE_EVENT_TYPE(kEvtTextModifiedChange);
DEFINE_EVENT_TYPE(kEvtCaretChange);

} } // namespace jil::editor
