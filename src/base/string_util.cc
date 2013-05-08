#include "base/string_util.h"

namespace base {

bool BoolFromString(const char* input, bool* output) {
  if (boost::iequals(input, kTrueString) || strcmp(input, "1") == 0) {
    *output = true;
    return true;
  } else if (boost::iequals(input, kFalseString) || strcmp(input, "0") == 0) {
    *output = false;
    return true;
  }
  return false;
}

bool BoolFromString(const char* input, bool default_output) {
  bool output = default_output;
  BoolFromString(input, &output);
  return output;
}

const char* StringFromBool(bool input) {
  return input ? kTrueString : kFalseString;
}

char GetLocalDecimalPoint() {
  char buf[4];
  sprintf(buf, "%.1f", 0.1);
  return buf[1];
}

static bool IsZeroDoubleString(const std::string& input) {
  char local_dp = GetLocalDecimalPoint();
  size_t size = input.size();
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] != local_dp && input[i] != '0') {
      return false;
    }
  }
  return true;
}

std::string LocalStringFromDouble(double d, int places, bool empty_for_zero) {
  if (places < 0) {
    places = 0;
  }
  char format[6];
  sprintf(format, "%%.%df", places);
  char buf[32] = { 0 };
  sprintf(buf, format, d);
  if (empty_for_zero && IsZeroDoubleString(buf)) {
    return "";
  }
  return buf;
}

std::string StdStringFromDouble(double d, int places, bool empty_for_zero) {
  std::string s = LocalStringFromDouble(d, places, empty_for_zero);
  if (!s.empty() && places > 0) {
    s[s.length() - places - 1] = '.';
  }
  return s;
}

} // namespace base
