#ifndef JIL_FONT_UTIL_H_
#define JIL_FONT_UTIL_H_
#pragma once

#include "wx/font.h"
#include "wx/string.h"

namespace jil {

// Get or create the font via wxTheFontList.
const wxFont& GetGlobalFont(int point_size,
                            const wxString& facename,
                            bool bold = false,
                            bool italic = false);

// Default and minimal point size of the font.
const int kDefaultFontSize = 11;
const int kMinFontSize = 6;

// Get a default preferred font.
wxString GetDefaultFont();

} // namespace jil

#endif // JIL_FONT_UTIL_H_
