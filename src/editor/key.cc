#include "editor/key.h"

namespace jil {\
namespace editor {\

wxString KeyCodeName(int key_code) {
  if (key_code >= 33 && key_code <= 126) {
    // Standard ASCII characters.
    return wxChar(key_code);
  } else {
    switch (key_code) {
    case WXK_BACK:
      return wxT("Back");
      // TODO
    }

    return wxEmptyString;
  }
}

static wxString& AddKeyPart(wxString& str, const wxString& part, const wxString& sep = wxT("+")) {
  if (!str.empty()) {
    str += sep;
  }
  str += part;
  return str;
}

wxString KeyModifiersName(int modifiers) {
  wxString name;

  if ((modifiers & wxMOD_WIN) != 0) {
    AddKeyPart(name, wxT("Win"));
  }
  // TODO
  if ((modifiers & wxMOD_CONTROL) != 0) {
#if defined (__WXMAC__)
    AddKeyPart(name, wxT("Cmd"));
#else
    AddKeyPart(name, wxT("Ctrl"));
#endif
  }
  if ((modifiers & wxMOD_ALT) != 0) {
    AddKeyPart(name, wxT("Alt"));
  }
  if ((modifiers & wxMOD_SHIFT) != 0) {
    AddKeyPart(name, wxT("Shift"));
  }

  return name;
}

wxString Key::ToString() const {
  wxString str;
  int leader_key = data_ >> 16;
  if (leader_key != 0) {
    str = KeyModifiersName(leader_key >> 8);
    AddKeyPart(str, KeyCodeName(leader_key & 0xff));
  }

  int key = data_ & 0xffff;
  if (key != 0) {
    AddKeyPart(str, KeyModifiersName(key >> 8), wxT(", "));
    AddKeyPart(str, KeyCodeName(key & 0xff));
  }

  return str;
}

} } // namespace jil::editor
