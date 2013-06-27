#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <boost/foreach.hpp>
#include "cpp_parser.h"
#include "htm_printer.h"
using namespace std;

const wstring typeToColor(WordType type)
{
  switch(type)
  {
    case kComment:
      return L"darkcyan";
    case kOperator:
      return L"BLACK";
    case kConstant:
      return L"MAGENTA";
    case kIdentifier:
      return L"darkgreen";
    case kStatement:
      return L"DARKSLATEBLUE";
    case kPackage:
      return L"BROWN";
    case kPreProc:
      return L"ROYALBLUE";
    case kType:
      return L"OLIVE";
    case kSpecial:
      return L"MEDIUMVIOLETRED";
    case kError:
      return L"RED";
    case kTodo:
      return L"ORANGE";
    default:
      return L"BLACK";
  }
  return L"BLACK";
}

const wstring typeToStr(WordType type)
{
  switch(type)
  {
    case kComment:
      return L"kComment";
    case kOperator:
      return L"kOperator";
    case kConstant:
      return L"kConstant";
    case kIdentifier:
      return L"kIdentifier";
    case kStatement:
      return L"kStatement";
    case kPackage:
      return L"kPackage";
    case kPreProc:
      return L"kPackage";
    case kType:
      return L"kType";
    case kSpecial:
      return L"kSpecial";
    case kError:
      return L"kError";
    case kTodo:
      return L"kTodo";
    default:
      return L"ERROR";
  }
  return L"ERROR";
}

int main()
{
  vector<wstring> lines;
  wstring str;
  while(getline(wcin, str))
  {
    // wcout << ">" << str << endl;
    lines.push_back(str);
  }
  // wcout << endl << endl << endl;

  CppParser cppParser;
  vector<vector<Word> > result;
  cppParser.parse(result, lines);
  assert(lines.size() == result.size());

  HtmPrinter htmPrinter("example.htm");
  for (int i = 0; i < lines.size(); i++)
  {
    BOOST_FOREACH(Word w, result[i])
    {
      wstring color = typeToColor(w.type);
      wstring type = typeToStr(w.type);
      htmPrinter.print(lines[i].substr(w.pos, w.length), color, type);
    }
    htmPrinter.endl();
  }
  wcout << L"Done! check: example.htm" << endl;

  return 0;
}
