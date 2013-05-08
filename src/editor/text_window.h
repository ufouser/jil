#ifndef JIL_EDITOR_TEXT_WINDOW_H_
#define JIL_EDITOR_TEXT_WINDOW_H_
#pragma once

#include "wx/scrolwin.h"
#include "editor/buffer_listener.h"
#include <vector>
#include <list>
#include <map>
#include <string>
#include <bitset>
//#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/compile_config.h"
#include "editor/option.h"
#include "editor/status_line.h"
#include "editor/seeker.h"
#include "editor/binding.h"
#include "editor/theme.h"

class wxTimer;
class wxTimerEvent;
class wxMouseCaptureLostEvent;

namespace jil {
namespace editor {

class Binding;
class LineNrArea;
class TextArea;
class StatusLine;
class TextBuffer;
class FileType;
class TextLine;
class TextExtent;
class WrapHelper;
class Renderer;
class Style;
class Options;

class EditAction;
class InsertStringAction;

// The window might have several intermediate sizes during the layout process.
// A size like (20, 20) doesn't make sense to trigger a re-wrap or refresh.
// And this constant is used in OnSize() for this optimization.
const int kUnreadyWindowSize = 100; // px

////////////////////////////////////////////////////////////////////////////////

class TextWindow : public wxScrolledWindow, public BufferListener {
  DECLARE_CLASS(TextWindow)
  DECLARE_EVENT_TABLE()

public:
  enum ThemeId {
    THEME_STATUS_LINE = 0,
    THEME_COUNT
  };

  enum ColorId {
    TEXT_BG = 0,
    COLOR_COUNT
  };

public:
  TextWindow(TextBuffer* buffer);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~TextWindow();

  //----------------------------------------------------------------------------

  void set_theme(const SharedTheme& theme) {
    theme_ = theme;
  }

  void set_style(Style* style) {
    style_ = style;
  }

  void set_binding(Binding* binding) {
    binding_ = binding;
  }

  //----------------------------------------------------------------------------

  // Override to set focus to text area.
  virtual void SetFocus() OVERRIDE;

  //----------------------------------------------------------------------------

  void SetTextFont(const wxFont& font);
  void SetLineNrFont(const wxFont& font);
  void SetStatusLineFont(const wxFont& font);

  void SetStatusFields(const std::vector<StatusLine::FieldInfo>& field_infos);

  //----------------------------------------------------------------------------
  // Undo/redo

  void Undo();
  void Redo();
  void Repeat();

  bool CanUndo() const;
  bool CanRedo() const;
  bool CanRepeat() const;

private:
  void Exec(EditAction* edit_action);

  //----------------------------------------------------------------------------
public:
  TextBuffer* buffer() const { return buffer_; }

  // Text buffer mediators:
  wxString buffer_file_name() const;
  bool buffer_modified() const;

  // Overriddens of BufferListener:
  virtual void OnBufferChange(ChangeType type, const ChangeData& data) OVERRIDE;
  virtual void OnBufferEncodingChange() OVERRIDE;
  virtual void OnBufferFileNameChange() OVERRIDE;
  virtual void OnBufferSaved() OVERRIDE;

  void HandleLineUpdated(const ChangeData& data);
  void HandleLineAdded(const ChangeData& data);
  void HandleLineDeleted(const ChangeData& data);

  //----------------------------------------------------------------------------
  // Wrap

  bool wrapped() const { return wrapped_; }
  void Wrap(bool wrapped);
private:
  // Return the wrap helper (create it if NULL).
  WrapHelper* wrap_helper() const;

  void AddWrapLines(const LineRange& line_range);
  void RemoveWrapLines(const LineRange& line_range);
  void UpdateWrapLines(const LineRange& line_range);

public:
  //----------------------------------------------------------------------------

  TextArea* text_area() const { return text_area_; }
  LineNrArea* line_nr_area() const { return line_nr_area_; }

  TextPoint caret_point() const { return caret_point_; }

  Key leader_key() const { return leader_key_; }
  void set_leader_key(Key key = Key());

  TextRange select_range() const { return select_range_; }

protected:
  //----------------------------------------------------------------------------

  void OnSize(wxSizeEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);

public:
  //----------------------------------------------------------------------------
  // Buffer change operations.
  // These functions create commands to change the buffer so that the change
  // can be undone later.

  void InsertChar(wchar_t c);
  void InsertString(const std::wstring& str);
  void InsertText(const std::wstring& text);

  void NewLineBelow();
  void NewLineAbove();

  void Move(TextUnit text_unit, SeekType seek_type);
  void Delete(TextUnit text_unit, SeekType seek_type);
  void Select(TextUnit text_unit, SeekType seek_type);
  // TODO
  void Change(const std::wstring& text);

  //----------------------------------------------------------------------------
  // Scroll.

  // wxWindow::ScrollLines() is very slow.
  // This function is implemented with wxWindow::Scroll() which is more straightforward.
  void ScrollLinesDirectly(int lines);

  void GoTo(Coord ln);

  //----------------------------------------------------------------------------

  // The number of lines a page contains.
  Coord GetPageSize() const;

private:
  //----------------------------------------------------------------------------
  // Delegated event handlers from TextArea.

  friend class TextArea;
  void OnTextPaint(Renderer& renderer);
  void DrawTextLine(Coord ln, Renderer& renderer, int x, int& y);
  void DrawWrappedTextLine(Coord ln, Renderer& renderer, int x, int& y);

  void OnTextSize(wxSizeEvent& evt);

  // Return true if the event is handled.
  bool OnTextMouse(wxMouseEvent& evt);

  // Return true if the event is handled.
  bool HandleTextLeftDown(wxMouseEvent& evt);
  void HandleTextLeftDown_NoAccel();
  void HandleTextLeftDown_Ctrl();
  void HandleTextLeftDown_CtrlAlt();
  void HandleTextLeftDown_CtrlShift();
  void HandleTextLeftDown_CtrlAltShift();
  void HandleTextLeftDown_Alt();
  void HandleTextLeftDown_Shift();
  void HandleTextLeftDown_AltShift();

  void HandleTextMouseMove(wxMouseEvent& evt);
  void SelectByDragging();

  void StartScrollTimer();
  void StopScrollTimer();
  void OnScrollTimer(wxTimerEvent& evt);

  void HandleTextMouseLeftUp(wxMouseEvent& evt);

  // Return true if the event is handled.
  bool HandleTextMouseWheel(wxMouseEvent& evt);

  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

  // Return true if the event is handled.
  bool OnTextKeyDown(wxKeyEvent& evt);

  void OnTextChar(wxKeyEvent& evt);

  //----------------------------------------------------------------------------
  // Delegated event handlers from LineNrArea.

  friend class LineNrArea;
  void OnLineNrPaint(wxDC& dc);
  void OnLineNrSize(wxSizeEvent& evt);

  //----------------------------------------------------------------------------

  // Renderer a text line.
  // Note: Tabs and different text styles are handled here.
  void RenderTextLine(Renderer& renderer, const TextLine& line, int x, int y);

  Coord LineFromScrolledY(int scrolled_y) const;

  // Calculate the caret point according to the text area client position.
  TextPoint CalcCaretPoint(const wxPoint& position);

  // Get the text client rect of the given line.
  // TODO: Rename to ClientRectFromLine
  //wxRect GetLineTextClientRect(Coord ln) const;

  // Get the client rect of text or line nr area from the given line range.
  wxRect ClientRectFromLineRange(wxWindow* area, const LineRange& line_range) const;

  wxRect ClientRectAfterLine(wxWindow* area, Coord ln, bool included) const;

  // Get the text client rect of the char with the given caret char position.
  //wxRect GetCharTextClientRect(Coord ln, Coord caret_x_char) const;

  // If the line is included, it will be refreshed together with the lines after it.
  void RefreshTextAfterLine(Coord ln, bool included, bool update = false);
  void RefreshLineNrAfterLine(Coord ln, bool included, bool update = false);

  // Refresh a line.
  // Sometimes, it's necessary to update immediately to avoid combined paint event.
  void RefreshTextByLine(Coord ln, bool update = false);
  void RefreshTextByLineRange(const LineRange& line_range, bool update = false);
  void RefreshLineNrByLine(Coord ln, bool update = false);
  void RefreshLineNrByLineRange(const LineRange& line_range, bool update = false);

  void HandleTextFontChange();
  void HandleLineNrFontChange();
  // Return true if areas are resized.
  bool HandleTextChange();

  void UpdateCharSize();
  void UpdateTextSize();
  void UpdateLineNrWidth();

  // Layout line number area, text area, etc.
  void LayoutAreas();

  void UpdateVirtualSize();

  // Set new caret point.
  // UpdateCaretPosition(scroll) is called to set caret position.
  void UpdateCaretPoint(const TextPoint& point, bool update_v = true, bool scroll = true);
  // Set caret position according to current caret point.
  // ScrollToCaret() is called if @scroll is true.
  void UpdateCaretPosition(bool scroll = true);
    // Scroll if necessary to make sure the caret is inside the client area.
  void ScrollToCaret();

  void SetSelection(const TextRange& range, TextDir select_dir);
  // Note: point_from might > point_to
  void SetSelection(const TextPoint& point_from, const TextPoint& point_to);
  void ClearSelection();
  // TODO: Rename
  void ExtendSelection(const TextPoint& point_to);

  // Get the width of the sub-line (substr(off1, off2 - off1)) of the given line.
  int GetLineWidth(const TextLine& line, size_t off1, size_t off2) const;
  int GetLineWidth(Coord ln, size_t off1, size_t off2) const;

  // Get the index of the char where the sub-line before it is with the given width.
  int GetCharIndex(Coord ln, int text_client_x) const;

  int GetWrappedCharIndex(Coord ln, int wrap_sub_ln, int text_client_x) const;

  int GetUnscrolledX(int scrolled_x) const;
  int GetUnscrolledY(int scrolled_y) const;
  int GetScrolledX(int unscrolled_x) const;
  int GetScrolledY(int unscrolled_y) const;

  // Get the range of lines currently displayed in the client area.
  // Note: The line number could be > actual line count.
  LineRange GetClientLineRange() const;

  //----------------------------------------------------------------------------

  FileType* file_type() const;

  // Notify parent about one of the following buffer changes.
  // - kEvtEncoding
  // - kEvtFileName
  // - kEvtTextModified
  void PostEvent(wxEventType evt_type);

private:
  TextBuffer* buffer_;
  // Remember the last "modified" state. Updated in OnBufferChange.
  bool buffer_modified_;

  // Edit options.
  Options options_;

  Style* style_;
  SharedTheme theme_;

  Binding* binding_;

  // Sub-windows and the GUI related properties.
  LineNrArea* line_nr_area_;
  TextArea* text_area_;
  StatusLine* status_line_;

  int line_nr_width_;

  // Char width is not very useful now, especially when the text font is not
  // mono-space or there are CJK characters. But just keep it.
  int char_width_;
  int char_height_;
  int line_height_;
  int text_width_;
  int text_height_;

  bool wrapped_; // FIXME: Use EditOption.wrap?
  mutable WrapHelper* wrap_helper_;

  TextExtent* text_extent_;

  // Caret coordinates.
  // There's always a char at the right of the caret. If the caret is at the
  // end of a line, the char is the line ending. For a given line with N chars,
  // caret_point_.x is in the range [0, N].
  // For a text with N lines, caret_point_.y is in the range [1, N].
  TextPoint caret_point_;

  // For multiple point editing.
  std::vector<TextPoint> extra_caret_points_;

  Coord v_caret_coord_x_;

  Key leader_key_;

  // Last text area mouse down point.
  TextPoint down_point_;
  // Last text area mouse move position.
  wxPoint move_position_;

  // Modifier keys pressed on mouse left down.
  int down_modifiers_;
  // Whether the mouse is dragged or not.
  bool dragged_;

  // For scrolling when mouse is dragged out of window.
  // TODO: Horizontal scroll should also be supported.
  wxTimer* scroll_timer_;

  enum ScrollDirection {
    kScrollUp,
    kScrollDown,
  };
  ScrollDirection scroll_dir_;

  // Current selection range.
  TextRange select_range_;
  // Current selection direction.
  TextDir select_dir_;
};

////////////////////////////////////////////////////////////////////////////////

// TODO:
// - File is deleted in disk
// - File becomes read-only or not in disk
// - File is changed by another application
// Rename
BEGIN_DECLARE_EVENT_TYPES()
// Buffer changes:
DECLARE_EVENT_TYPE(kEvtTextEncodingChange, 0) // File encoding changes
DECLARE_EVENT_TYPE(kEvtTextFileNameChange, 0) // File path name changes
DECLARE_EVENT_TYPE(kEvtTextModifiedChange, 0) // Text is modified
// View changes:
DECLARE_EVENT_TYPE(kEvtCaretChange, 0) // Caret position changes
END_DECLARE_EVENT_TYPES()

#define EVT_TEXT_ENCODING_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::editor::kEvtTextEncodingChange, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

#define EVT_TEXT_FILENAME_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::editor::kEvtTextFileNameChange, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

#define EVT_TEXT_MODIFIED_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::editor::kEvtTextModifiedChange, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

#define EVT_CARET_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::editor::kEvtCaretChange, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_WINDOW_H_
