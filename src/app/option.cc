#include "app/option.h"

namespace jil {\

const Option kOptions[] = {
  {   WRAP,             OPTION_BOOL,     false   },
  {   EXPAND_TAB,       OPTION_BOOL,     false   },
  {   SHOW_NUMBER,      OPTION_BOOL,     true    },
  {   SHOW_SPACE,       OPTION_BOOL,     false   },
  {   LINE_HIGHLIGHT,   OPTION_BOOL,     false   },
  {   TAB_STOP,         OPTION_INT,      4       },
};

} // namespace jil
