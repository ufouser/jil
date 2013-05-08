#ifndef JIL_EDITOR_DEFS_H_
#define JIL_EDITOR_DEFS_H_
#pragma once

#include "wx/defs.h"

namespace jil {
namespace editor {

enum TextDir {
  kForward = 0,
  kBackward
};

enum TextUnit {
  kChar = 0,
  kWord,
  kLine,
  kPage,
  kHalfPage,
  kBuffer,
  kSelected,
};

// Editor mode.
enum Mode {
  kNormalMode = 1,
  kVisualMode = 2, // Text selected. "Visual" is borrowed from Vim.
};
// FIXME: Rename as kBothModes?
#define kAllModes (kNormalMode | kVisualMode)

// File format (or EOL, End-Of-Line).
// See http://en.wikipedia.org/wiki/Newline
enum FileFormat {
  FF_NONE = 0,
  FF_WIN,
  FF_UNIX,
  FF_MAC,
};

enum ErrorCode {
  kNoError = 0,
  kIOError,
  kEncodingError
};

#if defined (__WXMSW__)
#  define FF_DEFAULT FF_WIN
#elif defined (__WXMAC__)
#  define FF_DEFAULT FF_MAC
#else
#  define FF_DEFAULT FF_UNIX
#endif

const wchar_t LF = L'\n'; // 0x0A
const wchar_t CR = L'\r'; // 0x0D

inline const std::wstring& GetEOL(FileFormat file_format) {
  static const std::wstring kEol[] = {
    L"",
    L"\r\n",
    L"\r",
    L"\n"
  };
  return kEol[file_format];
}

// Commonly used constant chars.
const wchar_t kTabChar = L'\t';
const wchar_t kSpaceChar = L' ';
const wchar_t kNilChar = L'\0';

// Jil uses Mozilla 'uchardet' to detect file encoding.
// 'uchardet' supports the following character sets:
// - ISO-8859-2 (European languages)
// - ISO-8859-5 (European languages)
// - ISO-8859-7 (European languages)
// - windows-1250
// - windows-1251
// - windows-1253
// - Big5 (Chinese)
// - EUC-JP (Japanese)
// - (X) EUC-KR (Korean)
// - (X) x-euc-tw (Chinese, EUC-TW)
// - HZ-GB2312 // TODO: gb18030 (Chinese)
// - (X) ISO-2022-CN (Chinese)
// - (X) ISO-2022-KR (Korean)
// - (X) ISO-2022-JP (Japanese)
// - UTF-8
// - Shift_JIS (Japanese)
// - UTF-16BE
// - UTF-16LE
// - KOI8-R (European languages, Russian)
// - x-mac-cyrillic (European languages)
// - (X) IBM866
// - (X) IBM855
// - TIS-620 (Thai) (ISO/IEC 8859-11)
// - ASCII (or latin1 ISO-8859-1) is also supported. But no name defined for it.

// References:
// ISO-2022: http://en.wikipedia.org/wiki/ISO/IEC_2022
// EUC: http://en.wikipedia.org/wiki/Extended_Unix_Code

// Here's a list of all supported encodings with names in lower case:
const char* const ASCII = "latin1"; // iso-8859-1
const char* const UTF_8 = "utf-8";
const char* const UTF_8_BOM = "utf-8 bom";
const char* const UTF_16BE = "utf-16be";
const char* const UTF_16LE = "utf-16le";
const char* const GB18030 = "gb18030";
const char* const BIG5 = "big5";
const char* const SHIFT_JIS = "shift_jis";
const char* const EUC_JP = "euc-jp";
const char* const KOI8_R = "koi8-r";
const char* const ISO_8859_2 = "iso-8859-2";
const char* const ISO_8859_5 = "iso-8859-5";
const char* const ISO_8859_7 = "iso-8859-7";
const char* const TIS_620 = "tis-620";
const char* const WINDOWS_1250 = "windows-1250";
const char* const WINDOWS_1251 = "windows-1251";
const char* const WINDOWS_1253 = "windows-1253";
const char* const X_MAC_CYRILLIC = "x-mac-cyrillic";

// BOM bytes.
const char* const UTF_8_BOM_BYTES = "\xEF\xBB\xBF";
const char* const UTF_16BE_BOM_BYTES = "\xFE\xFF";
const char* const UTF_16LE_BOM_BYTES = "\xFF\xFE";

} } // namespace jil::editor

#endif // JIL_EDITOR_DEFS_H_
