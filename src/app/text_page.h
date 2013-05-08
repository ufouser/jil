#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

// TextWindow as notebook page.

#include "editor/text_window.h"
#include "editor/notebook.h"
#include "base/compiler_specific.h"

namespace jil {\

namespace editor {\
class Style;
class Binding;
}

class TextPage : public editor::TextWindow, public editor::notebook::IPage {
  DECLARE_CLASS(TextPage)

public:
  TextPage(editor::TextBuffer* buffer);
  virtual ~TextPage();

  virtual wxWindow* NPage_AsWindow() OVERRIDE {
    return this;
  }

  virtual wxString NPage_GetLabel() const OVERRIDE;

  virtual bool NPage_Activate(bool active) OVERRIDE;
  virtual bool NPage_IsActive() const OVERRIDE;

  virtual bool NPage_IsModified() const OVERRIDE;

  virtual bool NPage_Close() OVERRIDE;
};

} // namespace jil

#endif // JIL_TEXT_PAGE_H_
