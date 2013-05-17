#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
using namespace std;
using namespace boost::algorithm;

static inline bool is_normal_word_ch(wchar_t ch)
{
  return iswalpha(ch) || iswdigit(ch) || L'_' == ch || ch > 127;
}

static void line_to_words(vector<wstring>& result, const wstring& line)
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
        result.push_back(line.substr(i - cnt, cnt));
      }
      // add current ch as special word
      result.push_back(line.substr(i, 1));

      // clear for next word
      cnt = 0;
    }
  }
  // the last one
  if (0 != cnt)
  {
    result.push_back(line.substr(line.length() - cnt, cnt));
  }
}

int main()
{
  wstring line = L".This,is a exa.,..mple23  23hah你好a";
  wcout << line << endl;
  vector<wstring> result;
  line_to_words(result, line);
  BOOST_FOREACH(wstring s, result)
  {
    wcout << ">=" << s << "<=" << endl;
  }
  cout << iswalpha(line[3]) << endl;
  cout << iswalpha(line[4]) << endl;
  cout << "Done" << endl;

  return 0;
}

