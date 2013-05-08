#include "app/active_list_window.h"
#include "wx/panel.h"
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "base/compiler_specific.h"

namespace jil {\

////////////////////////////////////////////////////////////////////////////////

class ActiveFilesPanel : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  ActiveFilesPanel(wxWindow* parent);

  void set_files(const std::vector<wxString>& files) {
    files_ = files;
  }

protected:
  virtual wxSize DoGetBestSize() const OVERRIDE;

  void OnPaint(wxPaintEvent& evt);

private:
  std::vector<wxString> files_;
};

ActiveFilesPanel::ActiveFilesPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

wxSize ActiveFilesPanel::DoGetBestSize() const {
  return wxSize(300, 500);
}

void ActiveFilesPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  for (size_t i = 0; i < files_.size(); ++i) {

  }
}

BEGIN_EVENT_TABLE(ActiveFilesPanel, wxPanel)
EVT_PAINT(ActiveFilesPanel::OnPaint)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ActiveListWindow, wxDialog)
EVT_ACTIVATE  (ActiveListWindow::OnActivate)
EVT_KEY_DOWN  (ActiveListWindow::OnKeyDown)
EVT_KEY_UP    (ActiveListWindow::OnKeyUp)
END_EVENT_TABLE()

ActiveListWindow::ActiveListWindow(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0) {
  files_panel_ = new ActiveFilesPanel(this);
  files_panel_->SetBackgroundColour(*wxRED);

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(files_panel_, 1, wxEXPAND | wxALL, 10);

  CenterOnParent();
  FitInside();
  //if (CanSetTransparent()) {
  //  SetTransparent(125);
  //}
}

void ActiveListWindow::set_files(const std::vector<wxString>& files) {
  files_panel_->set_files(files);
}

void ActiveListWindow::OnActivate(wxActivateEvent& evt) {
  if (!evt.GetActive()) {
    Close();
  }
  evt.Skip();
}

void ActiveListWindow::OnKeyDown(wxKeyEvent& evt) {
  const int key_code = evt.GetKeyCode();

  if (key_code == WXK_TAB) {

  }
  evt.Skip();
}

void ActiveListWindow::OnKeyUp(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_CONTROL) {
    Close();
  } else {
    evt.Skip();
  }
}

} // namespace jil
