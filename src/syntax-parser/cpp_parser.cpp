#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "cpp_parser.h"
using namespace std;
using namespace boost::algorithm;


void CppParser::test(const std::vector<std::wstring>& lines)
{
  wstring line = lines[0];
  wcout << line << endl;
  vector<Word> result;
  line_to_words(result, line);
  BOOST_FOREACH(Word w, result)
  {
    wcout << line.substr(w.pos, w.length) << "^";
  }
  wcout << endl;
}

void CppParser::test2(const std::vector<std::wstring>& lines)
{
  std::vector<std::vector<Word> > result;
  parse(result, lines);
  assert(lines.size() == result.size());
  for (int i = 0; i < lines.size(); i++)
  {
    wcout << "-" << i << "-";
    BOOST_FOREACH(Word w, result[i])
    {
      wcout << "^" << lines[i].substr(w.pos, w.length) ;
    }
    wcout << endl;
  }
}

/*
1. split
  split with these delimiters: [^a-zA-Z_0-9Chinese]
  the result should contains each delimiters
*/
void CppParser::line_to_words(std::vector<Word>& result, const std::wstring& line)
{
  size_t cnt = 0; // nr of successive chars for normal word
  for (size_t i = 0; i < line.length(); ++i)
  {
    // successive char for normal word
    if (is_normal_word_ch(line[i]))
    {
      ++cnt;
    }
    else
    {
      // there's normal word before this char
      if (0 != cnt)
      {
        Word word = {i - cnt, cnt};
        result.push_back(word);
      }
      // add current ch as special word
      Word word = {i, 1};
      result.push_back(word);

      // clear for next word
      cnt = 0;
    }
  }
  // the last one
  if (0 != cnt)
  {
    Word word = {line.length() - cnt, cnt};
    result.push_back(word);
  }
}

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
void CppParser::concat_adjacent_words_within_line(
  std::vector<Word>& result,
  const std::vector<Word>& input,
  const std::wstring& line)
{
  const size_t nrOfWords = input.size();
  size_t i = 0;
  while (i < nrOfWords)
  {
    // do nothing with last word
    if (i + 1 == nrOfWords)
    {
      result.push_back(input[i]);
      break;
    }
    wstring curr_word = word_to_string(input[i], line);
    wstring next_word = word_to_string(input[i+1], line);
    wstring two_words = curr_word + next_word;
    wstring three_words = two_words;
    if (i + 2 < nrOfWords)
    {
      three_words += word_to_string(input[i+2], line);
    }

    // 3-words concatenation: <<= >>=
    if (L"<<=" == three_words
      || L">>=" == three_words)
    {
      Word new_word = input[i];
      new_word.length += input[i+1].length + input[i+2].length;
      result.push_back(new_word);
      i += 3; // forword read iter by 3 words
    }
    // 2-words concatenation (excepts /?)
    else if (L"/*" == two_words
      || L"*/" == two_words
      || L"//" == two_words
      || (iswdigit(curr_word[0]) && L"." == next_word)
      || (L"#" == curr_word && iswalpha(next_word[0]))
      || L"++" == two_words
      || L"--" == two_words
      || L"::" == two_words
      || L"->" == two_words
      || L"<<" == two_words
      || L">>" == two_words
      || L"<=" == two_words
      || L">=" == two_words
      || L"==" == two_words
      || L"!=" == two_words
      || L"&&" == two_words
      || L"||" == two_words
      || L"+=" == two_words
      || L"-=" == two_words
      || L"*=" == two_words
      || L"/=" == two_words
      || L"%=" == two_words
      || L"&=" == two_words
      || L"|=" == two_words
      || L"!=" == two_words
      || L"  " == two_words
      || L"\t\t" == two_words)
    {
      Word new_word = input[i];
      new_word.length += input[i+1].length;
      result.push_back(new_word);
      i += 2; // forword read iter by 2 words
    }
    // 2-words concatenation for \?
    else if (L"\\" == curr_word)
    {
      Word new_word = input[i];
      new_word.length += 1; // merge next ch to current word
      result.push_back(new_word);

      if (input[i+1].length > 1)
      {
        Word new_word2 = input[i+1];
        new_word2.length -= 1; // merge next ch to current word
        result.push_back(new_word2);
      }
      i += 2;
    }
    else
    {
      result.push_back(input[i]);
      i++;
    }
  }
}

// merge words
const Word CppParser::merge_words(
const std::vector<Word>& words)
{
  assert(!words.empty());
  Word word = words[0];
  for (int i = 1; i < words.size(); ++i)
  {
    word.length += words[i].length;
  }
  return word;
}

bool CppParser::isStartingQuotedMode(const std::wstring& wordStr, std::wstring& endMarker)
{
  if (L"/*" == wordStr)
  {
    endMarker = L"*/";
    return true;
  }
  else if (L"//" == wordStr)
  {
    endMarker = L"";
    return true;
  }
  else if (L"\"" == wordStr)
  {
    endMarker = L"\"";
    return true;
  }
  else if (L"'" == wordStr)
  {
    endMarker = L"'";
    return true;
  }
  return false;
}

bool CppParser::is_word_any_of(const std::wstring& wordStr, const wchar_t* words[], int nrOfWords)
{
  for (int i = 0; i < nrOfWords; ++i)
  {
    if (wordStr == wstring(words[i]))
    {
      return true;
    }
  }
  return false;
}

WordType CppParser::strToType(const std::wstring& wordStr)
{
  if (wordStr.empty())
  {
    return kError;
  }

  // comments
  if (starts_with(wordStr, L"/*")
    || starts_with(wordStr, L"//"))
  {
    return kComment;
  }

  // constant
  const wchar_t* pConstKeywords[] = {
    L"true", L"false", L"NULL", L"Null", L"nil"
  };
  int nrOfConst = sizeof(pConstKeywords)/sizeof(pConstKeywords[0]);
  if (L'\"' == wordStr[0]
    || L'\'' == wordStr[0]
    || iswdigit(wordStr[0])
    || is_word_any_of(wordStr, pConstKeywords, nrOfConst))
  {
    return kConstant;
  }

  // preProc
  if (L'#' == wordStr[0])
  {
    return kPreProc;
  }

  // type
  const wchar_t* pTypeKeywords[] = {
    L"bool", L"auto", L"char", L"class", L"const",
    L"double", L"enum", L"explicit", L"export", L"extern",
    L"float", L"friend", L"inline", L"int", L"long",
    L"mutable", L"namespace", L"operator", L"private", L"protected",
    L"public", L"register", L"short", L"signed", L"static",
    L"struct", L"template", L"typedef", L"typename", L"union",
    L"unsigned", L"virtual", L"void", L"volatile", L"wchar_t"
  };
  int nrOfType = sizeof(pTypeKeywords) / sizeof(pTypeKeywords[0]);
  if (is_word_any_of(wordStr, pTypeKeywords, nrOfType))
  {
    return kType;
  }

  // statement
  const wchar_t* pStatKeywords[] = {
    L"asm", L"break", L"case", L"catch", L"continue",
    L"default", L"do", L"else", L"for", L"goto", L"if",
    L"return", L"switch", L"try", L"using", L"while"
  };
  int nrOfStat = sizeof(pStatKeywords) / sizeof(pStatKeywords[0]);
  if (is_word_any_of(wordStr, pStatKeywords, nrOfStat))
  {
    return kStatement;
  }

  // operators
  const wchar_t* pOperKeywords[] = {
    L"const_cast", L"delete", L"dynamic_cast", L"new", L"reinterpret_cast",
    L"sizeof", L"static_cast", L"this", L"throw", L"typeid",
    L"{", L"}", L";",
    L"(", L")", L"[", L"]", L"->", L".", L"::", L"++", L"--",
    L"!", L"~", L"++", L"--", L"-", L"+", L"*", L"&",
    L"->*", L".*", L"/", L"%",
    L"<<", L">>", L"<", L"<=", L">", L">=", L"==", L"!=",
    L"&", L"^", L"|", L"&&", L"||", L"?", L":",
    L"=", L"+=", L"-=", L"*=", L"/=", L"%=", L"&=",
    L"^=", L"|=", L"<<=", L">>=", L","
  };
  int nrOfOper = sizeof(pOperKeywords) / sizeof(pOperKeywords[0]);
  if (is_word_any_of(wordStr, pOperKeywords, nrOfOper))
  {
    return kOperator;
  }

  // special
  if (L' ' == wordStr[0]
    || L'\t' == wordStr[0])
  {
    return kSpecial;
  }

  // id
  if (is_normal_word_ch(wordStr[0]))
  {
    return kIdentifier;
  }
  return kError;
}

void CppParser::parse_one_line(
  vector<Word>& resultForCurrentLine,
  const std::wstring& currLine,
  bool& isQuotedMode,
  WordType& quotedType,
  std::wstring& endMarker)
{
  // step 1: split
  vector<Word> atomicWords;
  line_to_words(atomicWords, currLine);

  // step2: concatenate adjacent words within one line
  vector<Word> concatenatedWords;
  concat_adjacent_words_within_line(concatenatedWords,
    atomicWords, currLine);

  // do nothing for empty line
  if (concatenatedWords.empty())
  {
    return;
  }

  // buffer that holds words of quoted sentence
  vector<Word> mergingBuffer;

  for (int i = 0; i < concatenatedWords.size(); i++)
  {
    wstring currWordStr = currLine.substr(
      concatenatedWords[i].pos,
      concatenatedWords[i].length);
    Word word;
    word.pos = concatenatedWords[i].pos;
    word.length = concatenatedWords[i].length;

    // if under quoted matching mode (waiting for endMarker)
    if (isQuotedMode)
    {
      // set type and wait for merging
      word.type = quotedType;
      mergingBuffer.push_back(word);

      // if endmarkdr matched
      if (endMarker == currWordStr)
      {
        resultForCurrentLine.push_back(merge_words(mergingBuffer));
        mergingBuffer.clear();
        isQuotedMode = false;

        // endMarker detection
        if (endMarker == currWordStr || endMarker.empty()) // empty is for // comments
        {
          isQuotedMode = false;
        }
      }
    }// else: normal mode
    else
    {
      /*
quoted mode tests for / * // " ' */
      isQuotedMode = isStartingQuotedMode(currWordStr, endMarker);
      word.type = strToType(currWordStr);

      // if quoted mode starts, do more preparation
      if (isQuotedMode)
      {
        quotedType = word.type;
        mergingBuffer.push_back(word); // put to buffer for merging
      }
      // for normal words, just append it to final result
      else
      {
        resultForCurrentLine.push_back(word);
      }
    }
  }

  // deal with the un-ended quote-strings
  if (!mergingBuffer.empty())
  {
    assert(isQuotedMode);

    if (endMarker.empty()) // empty is for // comments
    {
      isQuotedMode = false;
    }
    else // multi-line strings
    {
      // for multi-line strings, do not turn off quotedMode
    }
    resultForCurrentLine.push_back(merge_words(mergingBuffer));
    mergingBuffer.clear();
  }
}

/* 3. concatenate sentence (cross lines) and mark type
  This function will call previous steps
- / * xxx * /
- //xxxEOL
- "xxx"
- 'xxx'
*/
void CppParser::parse(std::vector<std::vector<Word> >& result,
  const std::vector<std::wstring>& lines)
{
  bool isQuotedMode = false;
  WordType quotedType; // the type for the quoted sentence. e.g., comments, string
  wstring endMarker; // the key pattern that will ends the quoted sentence. e.g. */ " '

  for (size_t i = 0; i < lines.size(); i++)
  {
    const std::wstring currLine = lines[i];
    vector<Word> resultForCurrentLine;

    // step3: concatenate sentence (cross lines) and mark type
    parse_one_line(
      resultForCurrentLine,
      currLine,
      isQuotedMode,
      quotedType,
      endMarker);

    // put result: one vector per line
    result.push_back(resultForCurrentLine);
  }
}


/*
empty line test (below line)

*/
//int main()
int cpp_parser_test()
{
  // wstring line = L".This,is a exa.,..mple23 23haha";
  vector<wstring> lines;
  // lines.push_back(L"//there's hello word");
  // lines.push_back(L"haha haha dak");


  wstring str;
  while (getline(wcin, str))
  {
    wcout << ">" << str << endl;
    lines.push_back(str);
  }

  CppParser cppParser;
  //cppParser.test(lines);
  cppParser.test2(lines);

  return 0;
}
