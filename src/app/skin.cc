#include "app/skin.h"
#include "wx/mstream.h"
#include "wx/image.h"
#include "wx/stdpaths.h"
#include "wx/filename.h"
#include "wx/log.h"

namespace jil {
namespace skin {

static wxBitmap GetIconFromMemory(const unsigned char* data, size_t len) {
  wxMemoryInputStream is(data, len);
  wxImage icon(is, wxBITMAP_TYPE_PNG, -1);
  return wxBitmap(icon);
}

static wxString GetIconDir() {
  static wxString icon_dir;
  if (icon_dir.empty()) {
    icon_dir = wxStandardPaths::Get().GetResourcesDir() +
        wxFileName::GetPathSeparator() +
        _T("icon") +
        wxFileName::GetPathSeparator();
  }
  return icon_dir;
}

wxBitmap GetIcon(const wxChar* const icon_name, const wxChar* const icon_size) {
  wxBitmap icon(GetIconDir() + icon_name + icon_size + _T(".png"), wxBITMAP_TYPE_PNG);
  return icon;
}

} } // namespace jil::skin
