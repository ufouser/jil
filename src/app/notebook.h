#ifndef JIL_NOTEBOOK_H_
#define JIL_NOTEBOOK_H_
#pragma once

#include "editor/notebook.h"
#include "base/compiler_specific.h"

namespace jil {\

class Notebook : public editor::Notebook {
  DECLARE_EVENT_TABLE()

public:
  Notebook(const editor::SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~Notebook();

  virtual bool AddPage(editor::notebook::IPage* page, bool active = false) OVERRIDE;

protected:
  // Text window event handlers:
  void OnTextFileNameChange(wxCommandEvent& evt);
  void OnTextModifiedChange(wxCommandEvent& evt);
};

} // namespace jil

#endif // JIL_NOTEBOOK_H_
