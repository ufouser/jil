#include "editor/text_func.h"
#include "wx/clipbrd.h"
#include "wx/dataobj.h"
#include "wx/log.h"
#include "editor/text_buffer.h"
#include "editor/text_window.h"
#include "editor/action.h"

namespace jil {\
namespace editor {\

bool CanUndo(TextWindow* window) {
  return window->buffer()->CanUndo();
}

// Redo could be Repeat, but the user doesn't care about this.
bool CanRedo(TextWindow* window) {
  return window->buffer()->CanRedo() || window->buffer()->CanRepeat();
}

void ExecAction(TextWindow* window, Action* action) {
  // Note the returned action might be a new action.
  action = window->buffer()->AddAction(action);
  window->UpdateCaretPoint(action->point() + action->delta_point());
}

void MotionFunc::Exec(TextWindow* window) {
  TextBuffer* buffer = window->buffer();

  TextPoint point = window->caret_point();
  bool update_v = false;

  if (text_unit_ == kSelected) {
    if (!window->select_range().IsEmpty()) {
      if (seek_type_ == kBegin) {
        point = window->select_range().first_point();
      } else if (seek_type_ == kEnd) {
        point = window->select_range().last_point();
      }
    }
    update_v = true;
  } else {
    point = Seek(buffer, window->caret_point(), text_unit_, seek_type_);
    update_v = !(text_unit_ == kLine && (seek_type_ == kPrev || seek_type_ == kNext));
  }

  // Clear selection even won't move actually.
  // TODO: Vim allows switch caret between the ends of the visual text (o).
  window->ClearSelection();

  window->UpdateCaretPoint(point, update_v);
}

void DeleteFunc::Exec(TextWindow* window) {
  TextBuffer* buffer = window->buffer();

  if (text_unit_ == kSelected) {
    if (!window->select_range().IsEmpty()) {
      DeleteRangeAction* action = new DeleteRangeAction(buffer, window->select_range(), window->select_dir(), true);
      window->ClearSelection(/*refresh=*/false);
      ExecAction(window, action);
    }
  } else if (text_unit_ == kLine && seek_type_ == kWhole) {
    DeleteAction* action = new DeleteAction(buffer, TextPoint(0, window->caret_point().y), text_unit_, seek_type_);
    action->set_caret_point(window->caret_point());
    ExecAction(window, action);
  } else {
    // TODO
    if (Seek(buffer, window->caret_point(), text_unit_, seek_type_) != window->caret_point()) {
      ExecAction(window, new DeleteAction(buffer, window->caret_point(), text_unit_, seek_type_));
    }
  }
}

void ScrollFunc::Exec(TextWindow* window) {
  switch (text_unit_) {
    case kChar:
      // TODO: Scroll horizontally.
      if (seek_type_ == kPrev) {
      } else if (seek_type_ == kNext) {
      }
      break;

    case kLine:
      window->ScrollLines(seek_type_ == kPrev ? -1 : 1);
      break;

    case kHalfPage:
      {
        int lines = window->GetPageSize() / 2;
        window->ScrollLinesDirectly(seek_type_ == kPrev ? -lines : lines);
      }
      break;

    case kPage:
      window->ScrollPages(seek_type_ == kPrev ? -1 : 1);
      break;

    case kBuffer:
      // TODO
      //if (seek_type_ == kPrev) {
      //  window->GoTo(1);
      //} else if (seek_type_ == kNext) {
      //  window->GoTo(window->buffer()->LineCount());
      //}
      break;
  }
}

void SelectFunc::Exec(TextWindow* window) {
  TextBuffer* buffer = window->buffer();

  if (text_unit_ == kSelected && seek_type_ == kWhole) {
    // Clear current selection.
    window->ClearSelection();
    return;
  }

  if (text_unit_ == kBuffer && seek_type_ == kWhole) {
    // Select the whole buffer.
    Coord line_count = buffer->LineCount();
    window->SetSelection(TextPoint(0, 1), TextPoint(buffer->LineLength(line_count), line_count));
    return;
  }

  TextPoint point = Seek(buffer, window->caret_point(), text_unit_, seek_type_);
  if (point != window->caret_point()) {
    window->ExtendSelection(point);
  }
}

////////////////////////////////////////////////////////////////////////////////

// Clipboard

static std::wstring GetClipboardText() {
  std::wstring text;
  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
      wxTextDataObject data;
      wxTheClipboard->GetData(data);
      text = data.GetText().ToStdWstring();
    }
    wxTheClipboard->Close();
  }
  return text;
}

static bool SetClipboardText(const std::wstring& text) {
  if (!wxTheClipboard->Open()) {
    return false;
  }
  wxTheClipboard->SetData(new wxTextDataObject(wxString(text)));
  wxTheClipboard->Close();
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void InsertChar(TextWindow* window, wchar_t c) {
  TextBuffer* buffer = window->buffer();

  if (window->select_range().IsEmpty()) {
    InsertCharAction* insert_char = new InsertCharAction(buffer, window->caret_point(), c);
    buffer->AddInsertCharAction(insert_char);
    window->UpdateCaretPoint(insert_char->point() + insert_char->delta_point());
  } else {
    DeleteRangeAction* delete_range = new DeleteRangeAction(buffer, window->select_range(), window->select_dir(), true);
    delete_range->set_group_type(Action::kGroupBegin);
    window->ClearSelection();
    ExecAction(window, delete_range);

    InsertCharAction* insert_char = new InsertCharAction(buffer, window->caret_point(), c);
    insert_char->set_group_type(Action::kGroupEnd);
    buffer->AddInsertCharAction(insert_char);
    window->UpdateCaretPoint(insert_char->point() + insert_char->delta_point());
  }
}

void InsertString(TextWindow* window, const std::wstring& str) {
  TextBuffer* buffer = window->buffer();

  if (window->select_range().IsEmpty()) {
    ExecAction(window, new InsertStringAction(buffer, window->caret_point(), str));
  } else {
    DeleteRangeAction* dr = new DeleteRangeAction(buffer, window->select_range(), window->select_dir(), true);
    dr->set_group_type(Action::kGroupBegin);
    window->ClearSelection();
    ExecAction(window, dr);

    InsertStringAction* is = new InsertStringAction(buffer, window->caret_point(), str);
    is->set_group_type(Action::kGroupEnd);
    ExecAction(window, is);
  }
}

void Undo(TextWindow* tw) {
  while (true) {
    Action* action = tw->buffer()->Undo();
    if (action == NULL) {
      break;
    }

    DeleteRangeAction* delete_range_action = action->AsDeleteRangeAction();
    if (delete_range_action != NULL) {
      tw->SetSelection(delete_range_action->range(), delete_range_action->dir());
    } else {
      tw->ClearSelection();
      // TODO: Validate caret point.
      tw->UpdateCaretPoint(action->caret_point());
    }

    if (action->group_type() == Action::kGroupNone || action->group_type() == Action::kGroupBegin) {
      break;
    }
  }
}

void Redo(TextWindow* tw) {
  TextBuffer* buffer = tw->buffer();

  if (buffer->CanRedo()) {
    while (true) {
      Action* action = buffer->Redo();
      if (action == NULL) {
        break;
      }

      if (action->AsDeleteRangeAction() != NULL) {
        tw->ClearSelection();
      }
      tw->UpdateCaretPoint(action->point() + action->delta_point());

      if (action->group_type() == Action::kGroupNone || action->group_type() == Action::kGroupEnd) {
        break;
      }
    }
  } else if (buffer->CanRepeat()) {
    // TODO: Group type?
    tw->ClearSelection();
    Action* action = buffer->Repeat(tw->caret_point());
    if (action != NULL) {
      tw->UpdateCaretPoint(action->point() + action->delta_point());
    }
  }
}

void Cut(TextWindow* window) {
  TextBuffer* buffer = window->buffer();
  std::wstring text;

  if (!window->select_range().IsEmpty()) {
    text = buffer->GetText(window->select_range());
    DeleteFunc(kSelected, kWhole)(window);
  } else {
    // Cut the current line.
    TextPoint caret_point = window->caret_point();
    text = buffer->GetLine(caret_point.y).data() + GetEOL(FF_DEFAULT);

    if (caret_point.y >= buffer->LineCount()) {
      // Don't delete the line, just clear it.
      // TODO
    } else {
      DeleteFunc(kLine, kWhole)(window);
    }
  }

  SetClipboardText(text);
}

void Copy(TextWindow* window) {
  TextBuffer* buffer = window->buffer();
  std::wstring text;

  if (!window->select_range().IsEmpty()) {
    text = buffer->GetText(window->select_range());
  } else {
    // Copy the current line.
    TextPoint caret_point = window->caret_point();
    text = buffer->GetLine(caret_point.y).data() + GetEOL(FF_DEFAULT);
  }

  SetClipboardText(text);
}

void Paste(TextWindow* window) {
  std::wstring text = GetClipboardText();
  if (text.empty()) {
    return;
  }

  TextBuffer* buffer = window->buffer();

  if (window->select_range().IsEmpty()) {
    ExecAction(window, new InsertTextAction(buffer, window->caret_point(), text));
  } else {
    DeleteRangeAction* dr = new DeleteRangeAction(buffer, window->select_range(), window->select_dir(), true);
    dr->set_group_type(Action::kGroupBegin);
    window->ClearSelection();
    ExecAction(window, dr);

    InsertTextAction* it = new InsertTextAction(buffer, window->caret_point(), text);
    it->set_group_type(Action::kGroupEnd);
    ExecAction(window, it);
  }
}

void NewLineBreak(TextWindow* window) {
  InsertChar(window, LF);
}

void NewLineBelow(TextWindow* window) {
  window->ClearSelection();

  TextBuffer* buffer = window->buffer();

  // Insert '\n' at the end of current line.
  TextPoint p = window->caret_point();
  p.x = buffer->LineLength(p.y);
  InsertCharAction* ic = new InsertCharAction(buffer, p, LF);
  ic->set_caret_point(window->caret_point());
  buffer->AddInsertCharAction(ic);

  window->UpdateCaretPoint(ic->point() + ic->delta_point());
}

void NewLineAbove(TextWindow* window) {
  window->ClearSelection();

  TextBuffer* buffer = window->buffer();

  // Insert '\n' at the beginning of current line.
  TextPoint p(0, window->caret_point().y);
  InsertCharAction* ic = new InsertCharAction(buffer, p, LF);
  ic->set_caret_point(window->caret_point());
  ic->set_dir(kBackward);
  buffer->AddInsertCharAction(ic);

  window->UpdateCaretPoint(ic->point() + ic->delta_point());
}

void Wrap(TextWindow* window) {
  window->Wrap(!window->wrapped());
}

void ShowNumber(TextWindow* window) {
  window->ShowNumber(!window->options().show_number);
}
void ShowSpace(TextWindow* window) {
  window->ShowSpace(!window->options().show_space);
}

} } // namespace jil::editor
