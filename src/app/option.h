#ifndef JIL_OPTION_H_
#define JIL_OPTION_H_
#pragma once

#include <string>
#include <vector>
#include "boost/any.hpp"

namespace jil {\

// Option names. (Also used in config file.)
const char* const CJK = "cjk";
const char* const FILE_ENCODING = "file_encoding";
const char* const FONT = "font";
const char* const THEME = "theme";

const char* const WRAP = "wrap";
const char* const EXPAND_TAB = "expand_tab";
const char* const SHOW_NUMBER = "show_number";
const char* const SHOW_SPACE = "show_space";
const char* const LINE_HIGHLIGHT = "line_highlight";
const char* const TAB_STOP = "tab_stop";

enum OptionType {
  OPTION_INT,
  OPTION_BOOL,
  OPTION_STR,
};

struct Option {
  std::string name;
  OptionType type;
  boost::any value;

  template <typename T>
  T as() const {
    try {
      return boost::any_cast<T>(value);
    } catch (boost::bad_any_cast&) {
      return T();
    }
  }
};

extern const Option kOptions[];

} // namespace jil

#endif // JIL_OPTION_H_
