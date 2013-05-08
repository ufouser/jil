#ifndef JIL_SKIN_H_
#define JIL_SKIN_H_
#pragma once

// Jil has menus but doesn't have icons for their items.
// Jil has no toolbars.
// This is just like Sublime Text.

#include "wx/bitmap.h"

namespace jil {
namespace skin {

// Icon size strings.
const wxChar* const kIconSize16 = _T("16");
const wxChar* const kIconSize24 = _T("24");
const wxChar* const kIconSize32 = _T("32");
const wxChar* const kIconSize48 = _T("48");

const wxChar* const kIconNone = NULL;

// Note: The bitmap returned will be invalid if it doesn't exist.
wxBitmap GetIcon(const wxChar* const icon_name, const wxChar* const icon_size);

} } // namespace jil::skin

#endif // JIL_SKIN_H_
