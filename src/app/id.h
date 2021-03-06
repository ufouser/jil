#ifndef JIL_ID_H_
#define JIL_ID_H_
#pragma once

// Window ID, menu ID, etc.

#include "wx/defs.h"

namespace jil {\

enum WindowId {
  ID_NOTEBOOK = wxID_HIGHEST + 100,

  ID_WINDOW_LAST,
};

enum MenuId {
  ID_MENU_BEGIN = ID_WINDOW_LAST,

  // File
  ID_MENU_FILE_BEGIN = ID_MENU_BEGIN,
  ID_MENU_FILE_NEW = ID_MENU_FILE_BEGIN,
  ID_MENU_FILE_NEW_FROM_STUB,
  ID_MENU_FILE_NEW_WINDOW,
  ID_MENU_FILE_OPEN,
  ID_MENU_FILE_CLOSE,
  ID_MENU_FILE_CLOSE_ALL,
  ID_MENU_FILE_SAVE,
  ID_MENU_FILE_SAVE_AS,
  ID_MENU_FILE_SAVE_ALL,
  ID_MENU_FILE_END,

  // Edit
  ID_MENU_EDIT_BEGIN = ID_MENU_FILE_END,
  ID_MENU_EDIT_UNDO = ID_MENU_EDIT_BEGIN,
  ID_MENU_EDIT_REDO,
  ID_MENU_EDIT_CUT,
  ID_MENU_EDIT_COPY,
  ID_MENU_EDIT_PASTE,
  ID_MENU_EDIT_INSERT_FILE,
  ID_MENU_EDIT_WRAP,
#ifdef __WXDEBUG__
  ID_MENU_LOG_LINE_LENGTH_TABLE,
#endif // __WXDEBUG__
  ID_MENU_EDIT_END,

  // Tools
  ID_MENU_TOOLS_OPTIONS,

  ID_MENU_END,
};

} // namespace jil

#endif // JIL_ID_H_
