#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_
#pragma once

// char, wchar_t, std::string and std::wstring related utilities.
// Please don't put wxString related utilities here.

#include <string>
#include <algorithm>
#include <cassert>

#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/tokenizer.hpp"

#include "base/foreach.h"

namespace base {\

const char* const kTrueString = "true";
const char* const kFalseString = "false";

inline const char* SafeCString(const char* s) {
  return s == NULL ? "" : s;
}

inline std::string StdStringFromCString(const char* s) {
  return s == NULL ? "" : s;
}

// TODO: Use boost/algorithm/string/case_conv.hpp?
template <typename Char>
Char CharToUpper(Char c) {
  return std::use_facet<std::ctype<Char> >(std::locale()).toupper(c);
}

template <typename Char>
Char CharToLower(Char c) {
  return std::use_facet<std::ctype<Char> >(std::locale()).tolower(c);
}

template <typename Char>
std::basic_string<Char>& StringToUpper(std::basic_string<Char>& input) {
  std::transform(input.begin(), input.end(), input.begin(), CharToUpper<Char>);
  return input;
}

template <typename Char>
std::basic_string<Char> StringToUpperCopy(const std::basic_string<Char>& input) {
  std::basic_string<Char> output;
  output.resize(input.size());
  std::transform(input.begin(), input.end(), output.begin(), CharToUpper<Char>);
  return output;
}

template <typename Char>
std::basic_string<Char>& StringToLower(std::basic_string<Char>& input) {
  std::transform(input.begin(), input.end(), input.begin(), CharToLower<Char>);
  return input;
}

template <typename Char>
std::basic_string<Char> StringToLowerCopy(const std::basic_string<Char>& input) {
  std::basic_string<Char> output;
  output.resize(input.size());
  std::transform(input.begin(), input.end(), output.begin(), CharToLower<Char>);
  return output;
}

// ?
template <typename Char>
std::basic_string<Char> StringToUpperCopy(const Char* input) {
  std::basic_string<Char> output(input);
  StringToUpper(output);
  return output;
}

// ?
template <typename Char>
std::basic_string<Char> StringToLowerCopy(const Char* input) {
  std::basic_string<Char> output(input);
  StringToLower(output);
  return output;
}

template <typename Char>
std::basic_string<Char>& StringToTitle(std::basic_string<Char>& input) {
  if (!input.empty()) {
    input[0] = CharToUpper(input[0]);
  }
  return input;
}

// TODO: low others chars.
// TODO: multiple words: xxx yyy zzz -> Xxx Yyy Zzz
template <typename Char>
std::basic_string<Char> StringToTitleCopy(const std::basic_string<Char>& input) {
  std::basic_string<Char> output(input);
  StringToTitle(output);
  return output;
}

// Removes characters in trim_chars from the beginning and end of input.
template <typename Char>
std::basic_string<Char>& TrimString(std::basic_string<Char>& input,
                                    const Char trim_chars[]) {
  boost::algorithm::trim_if(input, boost::is_any_of(trim_chars));
  return input;
}

template <typename Char>
std::basic_string<Char> TrimStringCopy(const std::basic_string<Char>& input,
                                       const Char trim_chars[]) {
  return boost::algorithm::trim_copy_if(input, boost::is_any_of(trim_chars));
}

// ?
template <typename Char>
std::basic_string<Char> TrimStringCopy(const Char* input,
                                       const Char trim_chars[]) {
  return TrimStringCopy(std::basic_string<Char>(input), trim_chars);
}

// Usage:
//   LexicalCast<int>("123", 0);
//   LexicalCast<std::string>(123, "");
template <typename To, typename From>
To LexicalCast(const From& input, const To& default_output) {
  try {
    return boost::lexical_cast<To>(input);
  } catch (boost::bad_lexical_cast&) {
    return default_output;
  }
}

// "true"/"false" (case ignored) -> true/false
// "1"/"0" -> true/false (TBD)
bool BoolFromString(const char* input, bool* output);
bool BoolFromString(const char* input, bool default_output);
// true/false -> "true"/"false"
const char* StringFromBool(bool input);

char GetLocalDecimalPoint();

// \param places Decimal places (< 100).
// \param empty_for_zero If use empty string for zero value.
// \note The decimal point is the same as sprintf.
std::string LocalStringFromDouble(double d, int places, bool empty_for_zero);

// \param places Decimal places (< 100).
// \param empty_for_zero If use empty string for zero value.
// \note The decimal point is always '.'.
std::string StdStringFromDouble(double d, int places, bool empty_for_zero);

} // namespace base

#endif // BASE_STRING_UTIL_H_
