#ifndef JIL_ACTIVE_LIST_WINDOW_H_
#define JIL_ACTIVE_LIST_WINDOW_H_
#pragma once

// A floating window displaying opened files, active tool windows.
// Ctrl+Tab

#include "wx/dialog.h"
#include <vector>

namespace jil {\

class ActiveFilesPanel;

// A floating window displaying a list of active files.
class ActiveListWindow : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  ActiveListWindow(wxWindow* parent, wxWindowID id);

  void set_files(const std::vector<wxString>& files);

protected:
  void OnActivate(wxActivateEvent& evt);
  void OnKeyDown(wxKeyEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);

private:
  ActiveFilesPanel* files_panel_;
  std::vector<wxString> files_;
};

} // namespace jil

#endif // JIL_ACTIVE_LIST_WINDOW_H_
