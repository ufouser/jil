#ifndef JIL_EDITOR_TEXT_FUNC_H_
#define JIL_EDITOR_TEXT_FUNC_H_
#pragma once

#include "boost/function.hpp"
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/seeker.h"

namespace jil {\
namespace editor {\

class TextWindow;
class Action;

////////////////////////////////////////////////////////////////////////////////

// Text command function.
typedef boost::function<void (TextWindow*)> TextFunc;

bool CanUndo(TextWindow* window);
bool CanRedo(TextWindow* window);
void ExecAction(TextWindow* window, Action* action);

class SeekableFunc {
public:
  SeekableFunc(TextUnit text_unit, SeekType seek_type)
      : text_unit_(text_unit), seek_type_(seek_type) {
  }
  virtual ~SeekableFunc() {}

  void set_text_unit(TextUnit text_unit) { text_unit_ = text_unit; }
  void set_seek_type(SeekType seek_type) { seek_type_ = seek_type; }

  void operator()(TextWindow* window) {
    Exec(window);
  }

  virtual void Exec(TextWindow* window) = 0;

protected:
  TextUnit text_unit_;
  SeekType seek_type_;
};

class MotionFunc : public SeekableFunc {
public:
  MotionFunc(TextUnit text_unit, SeekType seek_type)
      : SeekableFunc(text_unit, seek_type) {
  }

  virtual void Exec(TextWindow* window) OVERRIDE;
};

class DeleteFunc : public SeekableFunc {
public:
  DeleteFunc(TextUnit text_unit, SeekType seek_type)
      : SeekableFunc(text_unit, seek_type) {
  }

  virtual void Exec(TextWindow* window) OVERRIDE;
};

class ScrollFunc : public SeekableFunc {
public:
  ScrollFunc(TextUnit text_unit, SeekType seek_type)
      : SeekableFunc(text_unit, seek_type) {
  }

  virtual void Exec(TextWindow* window) OVERRIDE;
};

class SelectFunc : public SeekableFunc {
public:
  SelectFunc(TextUnit text_unit, SeekType seek_type)
      : SeekableFunc(text_unit, seek_type) {
  }

  virtual void Exec(TextWindow* window) OVERRIDE;
};

void InsertChar(TextWindow* window, wchar_t c);
void InsertString(TextWindow* window, const std::wstring& str);

// Predefined text functions.
void Undo(TextWindow* window);
void Redo(TextWindow* window);
void Cut(TextWindow* window);
void Copy(TextWindow* window);
void Paste(TextWindow* window);
void NewLineBreak(TextWindow* window);
void NewLineBelow(TextWindow* window);
void NewLineAbove(TextWindow* window);
void Wrap(TextWindow* window);
void ShowNumber(TextWindow* window);
void ShowSpace(TextWindow* window);

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_FUNC_H_
