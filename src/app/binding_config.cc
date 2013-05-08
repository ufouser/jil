#include "app/binding_config.h"
#include <vector>
#include <cctype>
#include <exception>
#include "wx/log.h"
#include "base/foreach.h"
#include "editor/binding.h"
#include "editor/seeker.h"
#include "app/config.h"
#include "app/id.h"
#include "app/book_frame.h"
#include "app/binding.h"

namespace jil {\

using namespace editor;

namespace {\

void SwitchPage(BookFrame* book_frame) {
  book_frame->SwitchPage();
}

struct BookCmd {
  std::string name;
  BookFunc func;
};

struct MenuBookCmd {
  std::string name;
  int menu; // 0 if no menu for this book command.
};

// TODO
const BookCmd kBookCmds[] = {
  { "switch_page",  SwitchPage },
};

const MenuBookCmd kMenuBookCmds[] = {
  { "new", ID_MENU_FILE_NEW },
  { "new_from_stub", ID_MENU_FILE_NEW_FROM_STUB },
  { "new_window", ID_MENU_FILE_NEW_WINDOW },
  { "open", ID_MENU_FILE_OPEN },
  { "close", ID_MENU_FILE_CLOSE },
  { "close_all", ID_MENU_FILE_CLOSE_ALL },
  { "save", ID_MENU_FILE_SAVE },
  { "save_as", ID_MENU_FILE_SAVE_AS },
  { "save_all", ID_MENU_FILE_SAVE_ALL },
};

struct TextCmd {
  TextCmd(const std::string& name_, TextFunc func_)
      : name(name_), func(func_) {
  }
  std::string name;
  TextFunc func;
};

struct MenuTextCmd : public TextCmd {
  MenuTextCmd(const std::string& name_, TextFunc func_, int menu_)
      : TextCmd(name_, func_), menu(menu_) {
  }
  int menu;
};

const TextCmd kTextCmds[] = {
  TextCmd("new_line_break", NewLineBreak),
  TextCmd("new_line_below", NewLineBelow),
  TextCmd("new_line_above", NewLineAbove),
};

const MenuTextCmd kMenuTextCmds[] = {
  MenuTextCmd("undo", Undo, ID_MENU_EDIT_UNDO),
  MenuTextCmd("redo", Redo, ID_MENU_EDIT_REDO),
  MenuTextCmd("cut", Cut, ID_MENU_EDIT_CUT),
  MenuTextCmd("copy", Copy, ID_MENU_EDIT_COPY),
  MenuTextCmd("paste", Paste, ID_MENU_EDIT_PASTE),
  MenuTextCmd("wrap", Wrap, ID_MENU_EDIT_WRAP),
};

bool IsBookCmd(const std::string& name, BookFunc& func) {
  for (size_t i = 0; i < sizeof(kBookCmds) / sizeof(BookFunc); ++i) {
    if (name == kBookCmds[i].name) {
      func = kBookCmds[i].func;
      return true;
    }
  }
  return false;
}

bool IsMenuBookCmd(const std::string& name, int& menu) {
  for (size_t i = 0; i < sizeof(kMenuBookCmds) / sizeof(MenuBookCmd); ++i) {
    if (name == kMenuBookCmds[i].name) {
      menu = kMenuBookCmds[i].menu;
      return true;
    }
  }
  return false;
}

bool IsTextCmd(const std::string& name, TextFunc& func) {
  for (size_t i = 0; i < sizeof(kTextCmds) / sizeof(TextFunc); ++i) {
    if (name == kTextCmds[i].name) {
      func = kTextCmds[i].func;
      return true;
    }
  }
  return false;
}

bool IsMenuTextCmd(const std::string& name, TextFunc& func, int& menu) {
  for (size_t i = 0; i < sizeof(kMenuTextCmds) / sizeof(MenuTextCmd); ++i) {
    if (name == kMenuTextCmds[i].name) {
      func = kMenuTextCmds[i].func;
      menu = kMenuTextCmds[i].menu;
      return true;
    }
  }
  return false;
}


// Exception for key parsing error.
class KeyParseException : public std::exception {
public:
  KeyParseException(const std::string& what)
      : std::exception(what.c_str()) {
  }
};

TextUnit ParseUnit(const std::string& unit_str) throw(KeyParseException) {
  if (unit_str == "char") {
    return kChar;
  } else if (unit_str == "word") {
    return kWord;
  } else if (unit_str == "line") {
    return kLine;
  } else if (unit_str == "page") {
    return kPage;
  } else if (unit_str == "half_page") {
    return kHalfPage;
  } else if (unit_str == "buffer") {
    return kBuffer;
  } else if (unit_str == "selected") {
    return kSelected;
  } else {
    throw KeyParseException("unit=" + unit_str);
  }
}

SeekType ParseSeek(const std::string& seek_str) throw(KeyParseException) {
  if (seek_str == "whole") {
    return kWhole;
  } else if (seek_str == "begin") {
    return kBegin;
  } else if (seek_str == "end") {
    return kEnd;
  } else if (seek_str == "prev") {
    return kPrev;
  } else if (seek_str == "next") {
    return kNext;
  } else {
    throw KeyParseException("seek=" + seek_str);
  }
}

wxDirection ParseDir(const std::string& dir_str) throw(KeyParseException) {
  if (dir_str == "left") {
    return wxLEFT;
  } else if (dir_str == "right") {
    return wxRIGHT;
  } else if (dir_str == "up") {
    return wxUP;
  } else if (dir_str == "down") {
    return wxDOWN;
  } else {
    throw KeyParseException("dir=" + dir_str);
  }
}

int ParseMode(const std::string& mode_str) throw(KeyParseException) {
  if (mode_str.empty()) {
    return kAllModes;
  } else if (mode_str == "normal") {
    return kNormalMode;
  } else if (mode_str == "visual") {
    return kVisualMode;
  } else {
    throw KeyParseException("mode=" + mode_str);
  }
}

// TODO wxMOD_WIN
int ParseModifier(const std::string& modifier_str) throw(KeyParseException) {
  if (modifier_str == "ctrl") {
    return wxMOD_CONTROL;
  } else if (modifier_str == "alt") {
    return wxMOD_ALT;
  } else if (modifier_str == "shift") {
    return wxMOD_SHIFT;
  } else {
    throw KeyParseException("modifiers=" + modifier_str);
    //return wxMOD_NONE;
  }
}

// F1, F2, etc.
int ParseF(const std::string& keycode_str) {
  assert(keycode_str.size() > 1);

  if (keycode_str[0] != 'f' && keycode_str[0] != 'F') {
    return 0;
  }

  int num = std::atoi(&keycode_str[1]);
  if (num < 1 || num > 12) { // Only support F1 ~ F2.
    return 0;
  }

  return WXK_F1 + (num - 1);
}

#define kKeyCount 16

// SORTED special key names.
const char* kSortedKeyNames[kKeyCount] = {
  "back",
  "delete",
  "down",
  "end",
  "enter",
  "escape",
  "home",
  "insert",
  "left",
  "pagedown",
  "pageup",
  "return",
  "right",
  "space",
  "tab",
  "up",
};

// Special key codes SORTED by name.
const wxKeyCode kSortedKeyCodes[kKeyCount] = {
  WXK_BACK,
  WXK_DELETE,
  WXK_DOWN,
  WXK_END,
  WXK_RETURN,
  WXK_ESCAPE,
  WXK_HOME,
  WXK_INSERT,
  WXK_LEFT,
  WXK_PAGEDOWN,
  WXK_PAGEUP,
  WXK_RETURN,
  WXK_RIGHT,
  WXK_SPACE,
  WXK_TAB,
  WXK_UP,
};

size_t IndexKeyName(const char* keyname) {
  size_t i = 0;
  size_t j = kKeyCount;

  while (j > i) {
    size_t m = i + (j - i) / 2;
    int cmp = strcmp(keyname, kSortedKeyNames[m]);
    if (cmp == 0) {
      return m;
    } else if (cmp < 0) {
      j = m;
    } else {
      i = m + 1;
    }
  }

  return kKeyCount;
}

// "s" -> S, "F1" -> WXK_F1, home -> WXK_HOME, etc.
int ParseKeycode(const std::string& keycode_str) {
  if (keycode_str.empty()) {
    return 0;
  }

  if (keycode_str.size() == 1) {
    char keycode = keycode_str[0];
    if (std::isalpha(keycode) != 0) {
      keycode = toupper(keycode); // Virtual key is in upper case.
    }
    return keycode;
  }

  int keycode = ParseF(keycode_str);
  if (keycode != 0) {
    return keycode;
  }

  size_t i = IndexKeyName(keycode_str.c_str());
  if (i != kKeyCount) {
    return kSortedKeyCodes[i];
  }

  return 0;
}

// A command can be bound with multiple keys, separated with semicolon.
// Examples:
//  - delete;back ('delete' and 'back' are bound to the same command)
//  - ctrl+;;ctrl+i
// For the second example, the first simicolon is the keycode while the second
// semicolon is the key delimeter. That's why we don't use a general tokenizer.
void SplitKeys(const std::string& keys_str, std::vector<std::string>& splited_keys_str) {
  size_t p = 0;
  size_t i = 0;
  for (; i < keys_str.size(); ++i) {
    if (keys_str[i] == ';') {
      if (i > p) {
        // A semicolon after plus is not a separator.
        if (keys_str[i - 1] != '+') {
          splited_keys_str.push_back(keys_str.substr(p, i - p));
          p = i + 1;
        }
      }
    }
  }
  if (i > p) {
    splited_keys_str.push_back(keys_str.substr(p, i - p));
  }
}

bool ParseKeyStroke(const std::string& key_str, int& code, int& modifiers) {
  size_t offset = 0;
  while (true) {
    size_t plus = key_str.find_first_of('+', offset);
    if (plus != std::string::npos) {
      modifiers = modifiers | ParseModifier(key_str.substr(offset, plus - offset));
    } else {
      code = ParseKeycode(key_str.substr(offset));
      if (code == 0) {
        return false;
      }
      break;
    }
    offset = plus + 1;
  }
  return true;
}

bool ParseKey(const std::string& key_str, Key& key) {
  int code = 0;
  int leader_code = 0;
  int modifiers = 0;
  int leader_modifiers = 0;

  // Check leader key.
  size_t comma = key_str.find_first_of(',');
  if (comma != std::string::npos) {
    if (comma > 0 && key_str[comma - 1] == '+') {
      comma = key_str.find_first_of(',', comma + 1);
    }
  }

  if (comma != std::string::npos) {
    if (!ParseKeyStroke(key_str.substr(0, comma), leader_code, leader_modifiers)) {
      return false;
    }
    if (!ParseKeyStroke(key_str.substr(comma + 1), code, modifiers)) {
      return false;
    }
  } else { // No leader key.
    if (!ParseKeyStroke(key_str, code, modifiers)) {
      return false;
    }
  }

  key.Set(leader_code, leader_modifiers, code, modifiers);
  return true;
}

std::vector<Key> ParseKeys(const std::string& keys_str) throw(KeyParseException) {
  std::vector<Key> keys;
  std::vector<std::string> splited_keys_str;
  SplitKeys(keys_str, splited_keys_str);

  Key key;
  for (size_t i = 0; i < splited_keys_str.size(); ++i) {
    if (!ParseKey(splited_keys_str[i], key)) {
      throw KeyParseException("Failed to parse binding: " + keys_str);
    } else {
      keys.push_back(key);
    }
    key.Reset();
  }

  return keys;
}

bool ParseBindingItem(const Setting& kb_item, Binding* text_binding, BookBinding* book_binding) {
  try {
    int mode = ParseMode(kb_item.GetString("mode"));
    std::vector<Key> keys = ParseKeys(kb_item.GetString("key"));
    if (keys.empty()) {
      return false;
    }

    TextFunc text_func(NULL);
    std::string cmd = kb_item.GetString("cmd");

    if (cmd == "move") {
      text_func = MotionFunc(ParseUnit(kb_item.GetString("unit")), ParseSeek(kb_item.GetString("seek")));
    } else if (cmd == "delete") {
      text_func = DeleteFunc(ParseUnit(kb_item.GetString("unit")), ParseSeek(kb_item.GetString("seek")));
    } else if (cmd == "scroll") {
      text_func = ScrollFunc(ParseUnit(kb_item.GetString("unit")), ParseDir(kb_item.GetString("dir")));
    } else if (cmd == "select") {
      text_func = SelectFunc(ParseUnit(kb_item.GetString("unit")), ParseSeek(kb_item.GetString("seek")));
    } else {
      do {
        if (IsTextCmd(cmd, text_func)) {
          break;
        }

        int menu = 0;
        if (IsMenuTextCmd(cmd, text_func, menu)) {
          book_binding->AddTextFunc(text_func, keys[0]);
          text_func = NULL;
          book_binding->AddMenuKey(menu, keys[0]);
          break;
        }

        BookFunc book_func;
        if (IsBookCmd(cmd, book_func)) {
          // TODO
          break;
        }

        menu = 0;
        if (IsMenuBookCmd(cmd, menu)) {
          book_binding->AddMenuKey(menu, keys[0]);
          break;
        }
      } while (false);
    }

    if (text_func == NULL) { // Menu
      return true;
    }

    foreach (Key& key, keys) {
      text_binding->Bind(text_func, key, mode);
    }
  } catch (KeyParseException& e) {
    // TODO
    wxLogError(wxT("[Line %d] %s"), kb_item.line(), e.what());
    return false;
  }

  return true;
}

} // namespace

// TODO: Error handling. Log is not enough.
bool BindingConfig::Load(const wxString& file) {
  Config kb_config;
  if (!kb_config.Load(file)) {
    wxLogDebug(wxT("Failed to load binding file: %s"), file.c_str());
    return false;
  }

  Setting root_setting = kb_config.root();
  const int binding_list_num = root_setting.size();
  for (int i = 0; i < binding_list_num; ++i) {
    Setting list_setting = root_setting.Get(i);

    if (!list_setting || list_setting.type() != CONFIG_TYPE_LIST) {
      wxLogDebug(wxT("Binding items should be in a list."));
      continue;
    }

    const int binding_num = list_setting.size();
    for (int j = 0; j < binding_num; ++j) {
      ParseBindingItem(list_setting.Get(j), text_binding_, book_binding_);
    }
  }

  return true;
}

} // namespace jil
