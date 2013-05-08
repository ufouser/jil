#ifndef JIL_BINDING_H_
#define JIL_BINDING_H_
#pragma once

// Binding for menu and accelerators.

// TODO: Rename "BookBinding".

#include <string>
#include <vector>
#include <map>
#include "boost/function.hpp"
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/binding.h"

namespace jil {\

class BookFrame;

// Book command function.
typedef boost::function<void (BookFrame*)> BookFunc;

class BookBinding {
public:
  void AddTextFunc(editor::TextFunc text_func, editor::Key key);

  editor::TextFunc GetTextFunc(editor::Key key) const;
  editor::TextFunc GetTextFunc(int menu) const;

  bool IsLeaderKey(int code, int modifiers) const {
    return IsLeaderKey(editor::Key(code, modifiers));
  }
  bool IsLeaderKey(editor::Key key) const;

  void AddMenuKey(int menu, editor::Key key);
  editor::Key GetMenuKey(int menu) const;
  bool IsMenuKey(editor::Key key) const;

private:
  typedef std::map<editor::Key, editor::TextFunc> TextFuncMap;
  TextFuncMap text_funcs_;

  // Menu ID -> Key
  // TODO: mode
  typedef std::map<int, editor::Key> MenuKeyMap;
  MenuKeyMap menu_keys_;

  std::vector<editor::Key> leader_keys_;
};

} // namespace jil

#endif // JIL_BINDING_H_
