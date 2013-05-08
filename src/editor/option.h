#ifndef JIL_EDITOR_OPTION_H_
#define JIL_EDITOR_OPTION_H_
#pragma once

#include <string>
#include "wx/font.h"
#include "editor/defs.h"

namespace jil {\
namespace editor {\

enum LineHighlight {
  kHlNone = 0,
  kHlNumber,
  kHlText,
  kHlBoth,
};

// For fast option access (No key-value map).
class Options {
public:
  Options()
    : wrap(false)
    , tab_stop(4)
    , expand_tab(true)
    , show_number(true)
    , show_space(false)
    , line_highlight(kHlNumber) {
  }

  bool  wrap;
  int   tab_stop;
  bool  expand_tab;
  bool  show_number;
  bool  show_space;
  int   line_highlight;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_OPTION_H_
