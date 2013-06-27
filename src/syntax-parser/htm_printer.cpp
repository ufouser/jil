#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <boost/algorithm/string.hpp>
#include "htm_printer.h"

HtmPrinter::HtmPrinter(const char* path)
  :fout(path)
{
  assert(fout);
  fout << L"<body>\n";
}

HtmPrinter::~HtmPrinter()
{
  fout << L"</body>";
  fout.close();
}

void HtmPrinter::print(const std::wstring& str, const std::wstring& color, const std::wstring& comment)
{
  using namespace boost::algorithm;
  using std::wstring;

  // html encoding
  // "&" -> "&amp;"
  // "<" -> "&lt;"
  // ">" -> "&gt;"
  // " " -> "&nbsp;"
  // "\L" -> "&nbsp;&nbsp;&nbsp;&nbsp;"
  // "\n" -> "<br/>"
  wstring output = str;
  replace_all(output, L"&", L"&amp;");
  replace_all(output, L"<", L"&lt;");
  replace_all(output, L">", L"&gt;");
  replace_all(output, L" ", L"&nbsp;");
  replace_all(output, L"\t", L"&nbsp;&nbsp;&nbsp;&nbsp;");
  replace_all(output, L"\n", L"<br/>");
  if (!color.empty())
  {
    output = L"<font color=\"" + color + L"\">" + output + L"</font>";
  }
  if (!comment.empty())
  {
    output += L"<!--" + comment + L"-->";
  }
  
  fout << output;
}

void HtmPrinter::endl()
{
  fout << L"<br/>\n";
}

int htm_printer_test()
{
  // #include<boost/algorithm/string.h>
  HtmPrinter htmPrinter("a.htm");
  htmPrinter.print(L"#include", L"ROYALBLUE");
  htmPrinter.print(L" <", L"BLACK");
  htmPrinter.print(L"boost", L"GREEN");
  htmPrinter.print(L"/", L"BLACK");
  htmPrinter.print(L"algorithm", L"GREEN");
  htmPrinter.print(L"/", L"BLACK");
  htmPrinter.print(L"string", L"GREEN");
  htmPrinter.print(L".", L"BLACK");
  htmPrinter.print(L"h", L"GREEN");
  htmPrinter.print(L">", L"BLACK");
  htmPrinter.endl();

  //   /* hahahaha
  //   xxx*/
  htmPrinter.print(L"  /* hahahaha", L"AQUA");
  htmPrinter.endl();
  htmPrinter.print(L"  xxx*/", L"AQUA");
  htmPrinter.endl();


  // char* qq = "hello world";
  htmPrinter.print(L"  char", L"OLIVE");
  htmPrinter.print(L"* ", L"BLACK");
  htmPrinter.print(L"qq", L"GREEN");
  htmPrinter.print(L" = ", L"BLACK");
  htmPrinter.print(L"\"hello world\"", L"MAGENTA");
  htmPrinter.endl();

  // if (a > b)
  htmPrinter.print(L"  if ", L"DARKSLATEBLUE");
  htmPrinter.print(L"(", L"BLACK");
  htmPrinter.print(L"a", L"GREEN");
  htmPrinter.print(L" > ", L"BLACK");
  htmPrinter.print(L"b", L"GREEN");
  htmPrinter.print(L")", L"BLACK");
  htmPrinter.endl();

  return 0;
}
