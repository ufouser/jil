#include "editor/binding.h"
#include <algorithm>
#include "wx/clipbrd.h"
#include "wx/log.h"
#include "editor/text_buffer.h"
#include "editor/text_window.h"

namespace jil {\
namespace editor {\

void MotionFunc::operator()(TextWindow* text_window) {
  text_window->Move(text_unit_, seek_type_);
}

void DeleteFunc::operator()(TextWindow* text_window) {
  text_window->Delete(text_unit_, seek_type_);
}

void ScrollFunc::operator()(TextWindow* text_window) {
  switch (text_unit_) {
    case kChar:
      // TODO: Scroll horizontally.
      if (scroll_dir_ == wxLEFT) {
      } else if (scroll_dir_ == wxRIGHT) {
      }
      break;

    case kLine:
      text_window->ScrollLines(scroll_dir_ == wxUP ? -1 : 1);
      break;

    case kHalfPage:
      {
        int lines = text_window->GetPageSize() / 2;
        text_window->ScrollLinesDirectly(scroll_dir_ == wxUP ? -lines : lines);
      }
      break;

    case kPage:
      text_window->ScrollPages(scroll_dir_ == wxUP ? -1 : 1);
      break;

    case kBuffer:
      // TODO
      //if (scroll_dir_ == wxUP) {
      //  text_window->GoTo(1);
      //} else if (scroll_dir_ == wxDOWN) {
      //  text_window->GoTo(text_window->buffer()->LineCount());
      //}
      break;
  }
}

void SelectFunc::operator()(TextWindow* text_window) {
  text_window->Select(text_unit_, seek_type_);
}

void ChangeFunc::operator()(TextWindow* text_window) {
  text_window->Change(text_);
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

void Undo(TextWindow* text_window) {
  text_window->Undo();
}

void Redo(TextWindow* text_window) {
  if (text_window->CanRedo()) {
    text_window->Redo();
  } else if (text_window->CanRepeat()) {
    text_window->Repeat();
  }
}

void Cut(TextWindow* text_window) {
  TextBuffer* buffer = text_window->buffer();
  std::wstring text;

  if (!text_window->select_range().Empty()) {
    text = buffer->GetText(text_window->select_range());
    text_window->Delete(kSelected, kWhole);
  } else {
    // Cut the current line.
    TextPoint caret_point = text_window->caret_point();
    text = buffer->GetLine(caret_point.y).data() + GetEOL(FF_DEFAULT);

    if (caret_point.y >= buffer->LineCount()) {
      // Don't delete the line, just clear it.
      // TODO
    } else {
      text_window->Delete(kLine, kWhole);
    }
  }

  SetClipboardText(text);
}

void Copy(TextWindow* text_window) {
  TextBuffer* buffer = text_window->buffer();
  std::wstring text;

  if (!text_window->select_range().Empty()) {
    text = buffer->GetText(text_window->select_range());
  } else {
    // Copy the current line.
    TextPoint caret_point = text_window->caret_point();
    text = buffer->GetLine(caret_point.y).data() + GetEOL(FF_DEFAULT);
  }

  SetClipboardText(text);
}

void Paste(TextWindow* text_window) {
  std::wstring text = GetClipboardText();
  if (!text.empty()) {
    text_window->InsertText(text);
  }
}

void Wrap(TextWindow* text_window) {
  text_window->Wrap(!text_window->wrapped());
}

void NewLineBreak(TextWindow* text_window) {
  text_window->InsertChar(LF);
}

void NewLineBelow(TextWindow* text_window) {
  text_window->NewLineBelow();
}

void NewLineAbove(TextWindow* text_window) {
  text_window->NewLineAbove();
}

////////////////////////////////////////////////////////////////////////////////

TextFunc Binding::Find(Key key, Mode mode) const {
  const TextFuncMap& funcs = mode == kNormalMode ? normal_funcs_ : visual_funcs_;
  TextFuncMap::const_iterator it = funcs.find(key);
  if (it == funcs.end()) {
    return TextFunc();
  }
  return it->second;
}

bool Binding::IsLeaderKey(Key key) const {
  return std::find(leader_keys_.begin(), leader_keys_.end(), key) != leader_keys_.end();
}

void Binding::Bind(TextFunc text_func, Key key, int modes) {
  if ((modes & kNormalMode) != 0) {
    normal_funcs_[key] = text_func;
  }
  if ((modes & kVisualMode) != 0) {
    visual_funcs_[key] = text_func;
  }

  Key leader_key = key.leader();
  if (!leader_key.IsEmpty() && !IsLeaderKey(leader_key)) {
    leader_keys_.push_back(leader_key);
  }
}

} } // namespace jil::editor
