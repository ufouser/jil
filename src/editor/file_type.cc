#include "editor/file_type.h"
#include "wx/stdpaths.h"

namespace jil {\
namespace editor {\

//------------------------------------------------------------------------------

FileType::FileType(const wxString& name)
    : name_(name) {
}

FileType::~FileType() {
}

// TODO
void FileType::Load() {
  wxString user_data_dir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH;
  wxString file_type_dir = user_data_dir + wxT("ftplugin") + wxFILE_SEP_PATH + name_ + wxFILE_SEP_PATH;
}

} } // namespace jil::editor
