#include "base/string_util.h"
#include "gtest/gtest.h"

TEST(StringUtil, SafeCString) {
  const char* empty_str = base::SafeCString(NULL);
  EXPECT_NE(static_cast<const char*>(NULL), empty_str);
  EXPECT_EQ(0, strcmp(empty_str, ""));

  const char* str = base::SafeCString("test");
  EXPECT_EQ(0, strcmp(str, "test"));
}

TEST(StringUtil, StdStringFromCString) {
  EXPECT_EQ(std::string(""), base::StdStringFromCString(NULL));
  EXPECT_EQ(std::string("test"), base::StdStringFromCString("test"));
}

// Test:
//   bool BoolFromString(const char* input, bool* output);
TEST(StringUtil, BoolFromString1) {
  using base::BoolFromString;

  bool value = false;
  EXPECT_TRUE(BoolFromString("true", &value));
  EXPECT_EQ(true, value);

  value = false;
  EXPECT_TRUE(BoolFromString("TRUE", &value));
  EXPECT_EQ(true, value);

  value = false;
  EXPECT_TRUE(BoolFromString("1", &value));
  EXPECT_EQ(true, value);

  value = true;
  EXPECT_TRUE(BoolFromString("false", &value));
  EXPECT_EQ(false, value);

  value = true;
  EXPECT_TRUE(BoolFromString("FALSE", &value));
  EXPECT_EQ(false, value);

  value = true;
  EXPECT_TRUE(BoolFromString("0", &value));
  EXPECT_EQ(false, value);

  EXPECT_FALSE(BoolFromString("", &value));
  EXPECT_FALSE(BoolFromString("NOT A BOOL", &value));
}

// Test:
//   bool BoolFromString(const char* input, bool default_output);
TEST(StringUtil, BoolFromString2) {
  using base::BoolFromString;

  EXPECT_EQ(true, BoolFromString("true", false));
  EXPECT_EQ(true, BoolFromString("TRUE", false));
  EXPECT_EQ(true, BoolFromString("1", false));

  EXPECT_EQ(false, BoolFromString("false", true));
  EXPECT_EQ(false, BoolFromString("FALSE", true));
  EXPECT_EQ(false, BoolFromString("0", true));

  EXPECT_EQ(true, BoolFromString("", true));
  EXPECT_EQ(true, BoolFromString("NOT A BOOL", true));

  EXPECT_EQ(false, BoolFromString("", false));
  EXPECT_EQ(false, BoolFromString("NOT A BOOL", false));
}

TEST(StringUtil, CharToUpper) {
  using base::CharToUpper;
  EXPECT_EQ('A', CharToUpper('a'));
  EXPECT_EQ('A', CharToUpper('A'));
  EXPECT_EQ(L'A', CharToUpper(L'a'));
  EXPECT_EQ(L'A', CharToUpper(L'A'));
  EXPECT_EQ(L'中', CharToUpper(L'中'));
}

TEST(StringUtil, CharToLower) {
  using base::CharToLower;
  EXPECT_EQ('a', CharToLower('A'));
  EXPECT_EQ('a', CharToLower('a'));
  EXPECT_EQ(L'a', CharToLower(L'A'));
  EXPECT_EQ(L'a', CharToLower(L'a'));
  EXPECT_EQ(L'中', CharToLower(L'中'));
}

TEST(StringUtil, StringToUpper) {
  using base::StringToUpper;
  std::string str("aaa");
  EXPECT_EQ("AAA", StringToUpper(str));
  EXPECT_EQ("AAA", str);
  str = "AAA";
  EXPECT_EQ("AAA", StringToUpper(str));
  EXPECT_EQ("AAA", str);

  str = "";
  EXPECT_EQ("", StringToUpper(str));

  std::wstring wstr(L"aaa");
  EXPECT_EQ(L"AAA", StringToUpper(wstr));
  EXPECT_EQ(L"AAA", wstr);
  wstr = L"AAA";
  EXPECT_EQ(L"AAA", StringToUpper(wstr));
  EXPECT_EQ(L"AAA", wstr);

  wstr = L"";
  EXPECT_EQ(L"", StringToUpper(wstr));

  wstr = L"中文";
  EXPECT_EQ(L"中文", StringToUpper(wstr));
  EXPECT_EQ(L"中文", wstr);

  wstr = L"中a文";
  EXPECT_EQ(L"中A文", StringToUpper(wstr));
  EXPECT_EQ(L"中A文", wstr);

  wstr = L"中A文";
  EXPECT_EQ(L"中A文", StringToUpper(wstr));
  EXPECT_EQ(L"中A文", wstr);
}

TEST(StringUtil, StringToUpperCopy) {
  using base::StringToUpperCopy;

  EXPECT_EQ("AAA", StringToUpperCopy("aaa"));
  std::string str("aaa");
  EXPECT_EQ("AAA", StringToUpperCopy(str));
  EXPECT_EQ("aaa", str);

  EXPECT_EQ("AAA", StringToUpperCopy("AAA"));
  str = "AAA";
  EXPECT_EQ("AAA", StringToUpperCopy(str));
  EXPECT_EQ("AAA", str);

  EXPECT_EQ("", StringToUpperCopy(""));
  str = "";
  EXPECT_EQ("", StringToUpperCopy(str));

  EXPECT_EQ(L"AAA", StringToUpperCopy(L"aaa"));
  std::wstring wstr(L"aaa");
  EXPECT_EQ(L"AAA", StringToUpperCopy(wstr));
  EXPECT_EQ(L"aaa", wstr);

  EXPECT_EQ(L"AAA", StringToUpperCopy(L"AAA"));
  wstr = L"AAA";
  EXPECT_EQ(L"AAA", StringToUpperCopy(wstr));
  EXPECT_EQ(L"AAA", wstr);

  EXPECT_EQ(L"", StringToUpperCopy(L""));
  wstr = L"";
  EXPECT_EQ(L"", StringToUpperCopy(wstr));

  EXPECT_EQ(L"中文", StringToUpperCopy(L"中文"));
  wstr = L"中文";
  EXPECT_EQ(L"中文", StringToUpperCopy(wstr));
  EXPECT_EQ(L"中文", wstr);

  EXPECT_EQ(L"中A文", StringToUpperCopy(L"中a文"));
  wstr = L"中a文";
  EXPECT_EQ(L"中A文", StringToUpperCopy(wstr));
  EXPECT_EQ(L"中a文", wstr);

  EXPECT_EQ(L"中A文", StringToUpperCopy(L"中A文"));
  wstr = L"中A文";
  EXPECT_EQ(L"中A文", StringToUpperCopy(wstr));
  EXPECT_EQ(L"中A文", wstr);
}

TEST(StringUtil, StringToLower) {
  using base::StringToLower;
  std::string str("AAA");
  EXPECT_EQ("aaa", StringToLower(str));
  EXPECT_EQ("aaa", str);
  str = "aaa";
  EXPECT_EQ("aaa", StringToLower(str));
  EXPECT_EQ("aaa", str);

  str = "";
  EXPECT_EQ("", StringToLower(str));

  std::wstring wstr(L"AAA");
  EXPECT_EQ(L"aaa", StringToLower(wstr));
  EXPECT_EQ(L"aaa", wstr);
  wstr = L"aaa";
  EXPECT_EQ(L"aaa", StringToLower(wstr));
  EXPECT_EQ(L"aaa", wstr);

  wstr = L"";
  EXPECT_EQ(L"", StringToLower(wstr));

  wstr = L"中文";
  EXPECT_EQ(L"中文", StringToLower(wstr));
  EXPECT_EQ(L"中文", wstr);

  wstr = L"中A文";
  EXPECT_EQ(L"中a文", StringToLower(wstr));
  EXPECT_EQ(L"中a文", wstr);

  wstr = L"中a文";
  EXPECT_EQ(L"中a文", StringToLower(wstr));
  EXPECT_EQ(L"中a文", wstr);
}

TEST(StringUtil, StringToLowerCopy) {
  using base::StringToLowerCopy;

  EXPECT_EQ("aaa", StringToLowerCopy("aaa"));
  std::string str("AAA");
  EXPECT_EQ("aaa", StringToLowerCopy(str));
  EXPECT_EQ("AAA", str);

  EXPECT_EQ("aaa", StringToLowerCopy("aaa"));
  str = "aaa";
  EXPECT_EQ("aaa", StringToLowerCopy(str));
  EXPECT_EQ("aaa", str);

  EXPECT_EQ("", StringToLowerCopy(""));
  str = "";
  EXPECT_EQ("", StringToLowerCopy(str));

  EXPECT_EQ(L"aaa", StringToLowerCopy(L"AAA"));
  std::wstring wstr(L"AAA");
  EXPECT_EQ(L"aaa", StringToLowerCopy(wstr));
  EXPECT_EQ(L"AAA", wstr);

  EXPECT_EQ(L"aaa", StringToLowerCopy(L"aaa"));
  wstr = L"aaa";
  EXPECT_EQ(L"aaa", StringToLowerCopy(wstr));
  EXPECT_EQ(L"aaa", wstr);

  EXPECT_EQ(L"", StringToLowerCopy(L""));
  wstr = L"";
  EXPECT_EQ(L"", StringToLowerCopy(wstr));

  EXPECT_EQ(L"中文", StringToLowerCopy(L"中文"));
  wstr = L"中文";
  EXPECT_EQ(L"中文", StringToLowerCopy(wstr));
  EXPECT_EQ(L"中文", wstr);

  EXPECT_EQ(L"中a文", StringToLowerCopy(L"中A文"));
  wstr = L"中A文";
  EXPECT_EQ(L"中a文", StringToLowerCopy(wstr));
  EXPECT_EQ(L"中A文", wstr);

  EXPECT_EQ(L"中a文", StringToLowerCopy(L"中a文"));
  wstr = L"中a文";
  EXPECT_EQ(L"中a文", StringToLowerCopy(wstr));
  EXPECT_EQ(L"中a文", wstr);
}

// TODO
TEST(StringUtil, StringToTitle) {
  using base::StringToTitle;

  std::string str("aaa");
  EXPECT_EQ("Aaa", StringToTitle(str));
}

TEST(StringUtil, TrimString) {
  using base::TrimString;

  std::string str(" aaa ");
  EXPECT_EQ("aaa", TrimString(str, " "));
  EXPECT_EQ("aaa", str);

  str = "* aaa *";
  EXPECT_EQ("aaa", TrimString(str, " *"));
  EXPECT_EQ("aaa", str);

  str = "   ";
  EXPECT_EQ("", TrimString(str, " "));
  EXPECT_EQ("", str);

  str = "  * *";
  EXPECT_EQ("", TrimString(str, "* "));
  EXPECT_EQ("", str);
}

TEST(StringUtil, TrimStringCopy) {
  using base::TrimStringCopy;

  EXPECT_EQ("aaa", TrimStringCopy(" aaa ", " "));
  std::string str(" aaa ");
  EXPECT_EQ("aaa", TrimStringCopy(str, " "));
  EXPECT_EQ(" aaa ", str);

  EXPECT_EQ("aaa", TrimStringCopy("* aaa *", " *"));
  str = "* aaa *";
  EXPECT_EQ("aaa", TrimStringCopy(str, " *"));
  EXPECT_EQ("* aaa *", str);

  EXPECT_EQ("", TrimStringCopy("   ", " "));
  str = "   ";
  EXPECT_EQ("", TrimStringCopy(str, " "));
  EXPECT_EQ("   ", str);

  EXPECT_EQ("", TrimStringCopy("  * *", "* "));
  str = "  * *";
  EXPECT_EQ("", TrimStringCopy(str, "* "));
  EXPECT_EQ("  * *", str);
}

template <typename T>
static void TestLexicalCastWithInteger() {
  using base::LexicalCast;

  EXPECT_EQ("123", LexicalCast<std::string>(T(123), "0"));
  EXPECT_EQ("-123", LexicalCast<std::string>(T(-123), "0"));
  EXPECT_EQ("0", LexicalCast<std::string>(0, "-1"));
  EXPECT_EQ(123, LexicalCast<T>("123", 0));
  EXPECT_EQ(-123, LexicalCast<T>("-123", 0));
  EXPECT_EQ(0, LexicalCast<T>("0", -1));
  EXPECT_EQ(0, LexicalCast<T>("+0", -1));
  EXPECT_EQ(0, LexicalCast<T>("-0", -1));
  EXPECT_EQ(0, LexicalCast<T>("abc", 0));
}

TEST(StringUtil, LexicalCast) {
  TestLexicalCastWithInteger<int>();
  TestLexicalCastWithInteger<long>();
}

TEST(StringUtil, StdStringFromDouble) {
  using base::StdStringFromDouble;

  EXPECT_EQ("1.23", StdStringFromDouble(1.23456, 2, true));
  EXPECT_EQ("1.23456", StdStringFromDouble(1.23456, 5, true));
  EXPECT_EQ("1", StdStringFromDouble(1.23456, 0, true));
  EXPECT_EQ("1.23456000", StdStringFromDouble(1.23456, 8, true));
  EXPECT_EQ("1", StdStringFromDouble(1.23456, -1, true));

  EXPECT_EQ("0.00", StdStringFromDouble(0.001, 2, false));
  EXPECT_EQ("", StdStringFromDouble(0.001, 2, true));

}