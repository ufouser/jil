#ifndef JIL_BOOK_FRAME_H_
#define JIL_BOOK_FRAME_H_
#pragma once

#include "wx/frame.h"
#include <vector>
#include "wx/arrstr.h"
#include "wx/filename.h"
#include "base/compiler_specific.h"
#include "editor/theme.h"
#include "editor/binding.h"
#include "app/option.h"
#include "app/id.h"

namespace jil {\

namespace editor {\
class TextWindow;
class TextBuffer;
class FileType;
class Style;
class Binding;
}

class BookBinding;
class Session;
class TextPage;
class Notebook;
class ActiveListWindow;

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent);
bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent);

class BookFrame : public wxFrame {
  DECLARE_EVENT_TABLE()

public:
  enum ThemeId {
    THEME_NOTEBOOK = 0,
    THEME_TEXT_WINDOW,
    THEME_COUNT,
  };

  BookFrame(Session* session);

  // Must be called before Create().
  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  void set_style(editor::Style* style) {
    style_ = style;
  }

  void set_text_binding(editor::Binding* text_binding) {
    text_binding_ = text_binding;
  }

  void set_book_binding(BookBinding* book_binding) {
    book_binding_ = book_binding;
  }

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);
  virtual ~BookFrame();

  virtual bool Show(bool show = true) OVERRIDE;

  void OpenFile(const wxString& file_name, bool active);
  void OpenFiles(const wxArrayString& file_names);

protected: // TODO: public?
  // File menu operations.
  void FileNew();
  void FileNewFromStub();
  void FileNewWindow();
  void FileOpen();
  void FileClose();
  void FileCloseAll();
  void FileSave();
  void FileSaveAs();
  void FileSaveAll();

public:
  void SwitchPage();

protected:
  void OnActivate(wxActivateEvent& evt);
  void OnKeyDownHook(wxKeyEvent& evt);

  void OnMenuFile(wxCommandEvent& evt);
  void OnQuit(wxCommandEvent& evt);

  void OnMenuEdit(wxCommandEvent& evt);

  void OnToolsOptions(wxCommandEvent& evt);

  void OnHelp(wxCommandEvent& evt);

  // Update UI.
  // Update the enable state and, if necessary, the label of menu items.
  void OnUpdateFileUI(wxUpdateUIEvent& evt);
  void OnUpdateEditUI(wxUpdateUIEvent& evt);

  void OnClose(wxCloseEvent& evt);

private:
  void NewMenuItem(wxMenu* menu, int id, const wxString& label, wxItemKind item_kind = wxITEM_NORMAL);
  void LoadMenus();

  bool CheckLeaderKey(int menu_id);

  // Create text page for the given text buffer.
  TextPage* CreateTextPage(editor::TextBuffer* buffer);

  TextPage* ActiveTextPage() const;
  editor::TextBuffer* ActiveBuffer() const;

  TextPage* GetTextPage(const wxFileName& fn_object) const;

  void DoSaveBuffer(editor::TextBuffer* buffer);

private:
  Session* session_;

  Notebook* notebook_;

  editor::Style* style_;
  editor::SharedTheme theme_;

  editor::Binding* text_binding_;
  BookBinding* book_binding_;

  editor::Key down_key_;

  ActiveListWindow* active_list_window_;
};

} // namespace jil

#endif // JIL_BOOK_FRAME_H_
