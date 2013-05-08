#include "app/text_page.h"
#include "wx/msgdlg.h"
#include "editor/text_buffer.h"
#include "app/book_frame.h"
#include "app/i18n_strings.h"

namespace jil {\

using namespace editor;

IMPLEMENT_CLASS(TextPage, TextWindow)

TextPage::TextPage(TextBuffer* buffer)
    : TextWindow(buffer) {
}

TextPage::~TextPage() {
}

wxString TextPage::NPage_GetLabel() const {
  return buffer()->file_name();
}

bool TextPage::NPage_Activate(bool active) {
  Show(active);
  SetFocus();
  return true;
}

bool TextPage::NPage_IsActive() const {
  return IsShown();
}

bool TextPage::NPage_IsModified() const {
  return TextWindow::buffer()->modified();
}

bool TextPage::NPage_Close() {
  TextBuffer* buffer = TextWindow::buffer();

  if (!buffer->modified()) {
    return true;
  }

  wxString confirm_msg;
  if (buffer->new_created()) {
    confirm_msg = _("The file is untitled and changed, save it?");
  } else {
    confirm_msg = wxString::Format(_("The file (%s) has been changed, save it?"), NPage_GetLabel().c_str());
  }
  // Confirm save.
  int confirm_result = wxMessageBox(
      confirm_msg,
      _("Save File"),
      wxYES | wxNO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE);
  if (confirm_result == wxCANCEL) {
    // Don't close.
    return false;
  }

  if (confirm_result == wxNO) {
    // Don't save, close directly.
    return true;
  }

  // Save.
  if (buffer->new_created() || buffer->read_only()) {
    if (SaveBufferAs(buffer, wxGetTopLevelParent(this))) {
      return true;
    }
  } else {
    if (SaveBuffer(buffer, wxGetTopLevelParent(this))) {
      return true;
    }
  }

  // Failed to save.
  return false;
}

} // namespace jil
