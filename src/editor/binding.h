#ifndef JIL_EDITOR_BINDING_H_
#define JIL_EDITOR_BINDING_H_
#pragma once

// Binding, or key mapping, binds key(s) to command.

#include <string>
#include <vector>
#include <map>
#include "boost/function.hpp"
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/key.h"
#include "editor/seeker.h"

namespace jil {
namespace editor {

class TextWindow;
class Binding;

////////////////////////////////////////////////////////////////////////////////

// Text command function.
typedef boost::function<void (TextWindow*)> TextFunc;

class MotionFunc {
public:
  MotionFunc(TextUnit text_unit, SeekType seek_type)
      : text_unit_(text_unit), seek_type_(seek_type) {
  }

  void operator()(TextWindow* text_window);

private:
  TextUnit text_unit_;
  SeekType seek_type_;
};

class DeleteFunc {
public:
  DeleteFunc(TextUnit text_unit, SeekType seek_type)
      : text_unit_(text_unit), seek_type_(seek_type) {
  }

  void operator()(TextWindow* text_window);

private:
  TextUnit text_unit_;
  SeekType seek_type_;
};

class ScrollFunc {
public:
  ScrollFunc(TextUnit text_unit, wxDirection scroll_dir)
      : text_unit_(text_unit), scroll_dir_(scroll_dir) {
  }

  void operator()(TextWindow* text_window);

private:
  TextUnit text_unit_;
  wxDirection scroll_dir_;
};

class SelectFunc {
public:
  SelectFunc(TextUnit text_unit, SeekType seek_type)
      : text_unit_(text_unit), seek_type_(seek_type) {
  }

  void operator()(TextWindow* text_window);

private:
  TextUnit text_unit_;
  SeekType seek_type_;
};

// TODO
class ChangeFunc {
public:
  ChangeFunc(const std::wstring& text)
      : text_(text) {
  }

  void operator()(TextWindow* text_window);

private:
  std::wstring text_;
};

// Predefined text commands.
void Undo(TextWindow* text_window);
void Redo(TextWindow* text_window);
void Cut(TextWindow* text_window);
void Copy(TextWindow* text_window);
void Paste(TextWindow* text_window);
void Wrap(TextWindow* text_window);
void NewLineBreak(TextWindow* text_window);
void NewLineBelow(TextWindow* text_window);
void NewLineAbove(TextWindow* text_window);

class Binding {
public:
  TextFunc Find(Key key, Mode mode) const;

  bool IsLeaderKey(int code, int modifiers) const {
    return IsLeaderKey(Key(code, modifiers));
  }
  bool IsLeaderKey(Key key) const;

  // Bind a key to a text command.
  void Bind(TextFunc text_func, Key key, int modes);

private:
  typedef std::map<Key, TextFunc> TextFuncMap;
  // Keep command functions of each mode in separate maps for the
  // convenience of searching.
  TextFuncMap normal_funcs_;
  TextFuncMap visual_funcs_;

  // Store leader keys separately for fast checking.
  std::vector<Key> leader_keys_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_BINDING_H_
