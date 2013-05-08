#include "app/notebook.h"
#include "base/foreach.h"
#include "editor/text_window.h"

namespace jil {\

BEGIN_EVENT_TABLE(Notebook, wxPanel)
END_EVENT_TABLE()

Notebook::Notebook(const editor::SharedTheme& theme)
    : editor::Notebook(theme) {
}

bool Notebook::Create(wxWindow* parent, wxWindowID id) {
  if (!editor::Notebook::Create(parent, id)) {
    return false;
  }

  return true;
}

Notebook::~Notebook() {
  foreach (Tab* tab, tabs_) {
    Disconnect(tab->page->NPage_AsWindow()->GetId());
  }
}

bool Notebook::AddPage(editor::notebook::IPage* page, bool active) {
  bool added = editor::Notebook::AddPage(page, active);

  Connect(page->NPage_AsWindow()->GetId(),
          editor::kEvtTextFileNameChange,
          wxCommandEventHandler(Notebook::OnTextFileNameChange));
  Connect(page->NPage_AsWindow()->GetId(),
          editor::kEvtTextModifiedChange,
          wxCommandEventHandler(Notebook::OnTextModifiedChange));

  return added;
}

void Notebook::OnTextFileNameChange(wxCommandEvent& evt) {
  // Update page tab.
  wxObject* evt_obj = evt.GetEventObject();
  wxWindow* page_window = wxDynamicCast(evt_obj, wxWindow);
  if (page_window == NULL) {
    return;
  }

  Tab* tab = GetTabByWindow(page_window);
  if (tab == NULL) {
    return;
  }

  // Update page info.
  tab->size = CalcTabBestSize(tab->page->NPage_GetLabel());

  RefreshTabArea();
}

void Notebook::OnTextModifiedChange(wxCommandEvent& evt) {
  // Update page tab.
  wxObject* evt_obj = evt.GetEventObject();
  wxWindow* page_window = wxDynamicCast(evt_obj, wxWindow);
  if (page_window == NULL) {
    return;
  }

  Tab* tab = GetTabByWindow(page_window);
  if (tab == NULL) {
    return;
  }

  tab->modified = tab->page->NPage_IsModified();
  RefreshTabArea();
}

} // namespace jil
