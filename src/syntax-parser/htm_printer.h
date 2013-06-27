#ifndef HTM_PRINTER_H
#define HTM_PRINTER_H

#include <string>
#include <fstream>

class HtmPrinter
{
private:
public:
  HtmPrinter(const char* path);
  ~HtmPrinter();
  void print(const std::wstring& str, const std::wstring& color = L"", const std::wstring& comment = L"");
  void endl();
private:
  std::wofstream fout;
};

#endif
