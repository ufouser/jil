#ifndef CPP_PARSER_H
#define CPP_PARSER_H

#include <iostream>
#include <vector>
#include <string>
#include <cctype>


enum WordType
{
  kComment,
  kOperator,
  kConstant,
  kIdentifier,
  kStatement,
  kPackage,
  kPreProc,
  kType,
  kSpecial,
  kError,
  kTodo
};

struct Word
{
  size_t pos;
  size_t length;
  WordType type;
};

class CppParser
{
public:

  /* 3. concatenate sentence (cross lines) and mark type
    This function will call previous steps
  - / * xxx * /
  - //xxxEOL
  - "xxx"
  - 'xxx'
  */
  static void parse(std::vector<std::vector<Word> >& result,
    const std::vector<std::wstring>& lines);

private:

  static inline const std::wstring word_to_string(const Word& word, const std::wstring& line)
  {
    return line.substr(word.pos, word.length);
  }

  // is this char a normal char
  static inline bool is_normal_word_ch(wchar_t ch)
  {
    return iswalpha(ch) || iswdigit(ch) || L'_' == ch || ch > 127;
  }

  // does %words[] contains %wordStr
  static bool is_word_any_of(const std::wstring& wordStr, const wchar_t* words[], int nrOfWords);

  // merge several continuous words to one
  static const Word merge_words(const std::vector<Word>& words);

  // check whether should we start quoted mode matching
  static bool isStartingQuotedMode(const std::wstring& wordStr, std::wstring& endMarker);

private:

  /*
  1. split
    split with these delimiters: [^a-zA-Z_0-9Chinese]
    the result should contains each delimiters
  */
  static void line_to_words(std::vector<Word>& result, const std::wstring& line);

  /*
    2. concatenate adjacent words within one line
    - "/ *"
    - "* /"
    - "//"
    - [0-9]+ followed by "."--> i.e. -32.56e-18f --> - 32. 56e - 18f
    - "#" followed by [a-z]+
    - ++ -- :: -> << >> <= >= == != && || += -= *= /= %= &= |= ^= <<= >>=
    - "\?"
  */
  static void concat_adjacent_words_within_line(
    std::vector<Word>& result,
    const std::vector<Word>& input,
    const std::wstring& line);

  // type classification
  static WordType strToType(const std::wstring& wordStr);

  // portal for one line 
  static void parse_one_line(
    std::vector<Word>& resultForCurrentLine,
    const std::wstring& currLine,
    bool& isQuotedMode,
    WordType& quotedType,
    std::wstring& endMarker);

public:
  static void test(const std::vector<std::wstring>& lines);
  static void test2(const std::vector<std::wstring>& lines);

};

#endif
