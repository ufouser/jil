#include "app/binding.h"
#include <algorithm>
#include "wx/log.h"
#include "editor/text_buffer.h"
#include "editor/text_window.h"

namespace jil {\

using namespace editor;

TextFunc BookBinding::GetTextFunc(Key key) const {
  TextFuncMap::const_iterator it = text_funcs_.find(key);
  if (it == text_funcs_.end()) {
    return TextFunc();
  }
  return it->second;
}

TextFunc BookBinding::GetTextFunc(int menu) const {
  Key menu_key = GetMenuKey(menu);
  if (!menu_key.IsEmpty()) {
    return GetTextFunc(menu_key);
  }
  return TextFunc();
}

bool BookBinding::IsLeaderKey(Key key) const {
  return std::find(leader_keys_.begin(), leader_keys_.end(), key) != leader_keys_.end();
}

void BookBinding::AddTextFunc(TextFunc text_func, Key key) {
  text_funcs_[key] = text_func;

  Key leader_key = key.leader();
  if (!leader_key.IsEmpty() && !IsLeaderKey(leader_key)) {
    leader_keys_.push_back(leader_key);
  }
}

void BookBinding::AddMenuKey(int menu, Key key) {
  menu_keys_[menu] = key;
}

Key BookBinding::GetMenuKey(int menu) const {
  MenuKeyMap::const_iterator it = menu_keys_.find(menu);
  if (it == menu_keys_.end()) {
    return Key();
  }
  return it->second;
}

bool BookBinding::IsMenuKey(Key key) const {
  MenuKeyMap::const_iterator it = menu_keys_.begin();
  for (; it != menu_keys_.end(); ++it) {
    if (it->second == key) {
      return true;
    }
  }
  return false;
}

} // namespace jil
