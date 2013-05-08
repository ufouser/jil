#include "app/book_frame.h"

#include <fstream>
#include <string>
#include <vector>

#include "wx/sizer.h"
#include "wx/image.h"
#include "wx/menu.h"
#include "wx/toolbar.h"
#include "wx/filedlg.h"
#include "wx/msgdlg.h"
#include "wx/stdpaths.h"
#include "wx/dir.h"
#include "wx/dnd.h"
#include "wx/log.h"

#include "base/foreach.h"

#include "editor/option.h"
#include "editor/lex.h"
#include "editor/style.h"
#include "editor/text_buffer.h"
#include "editor/file_type.h"
#include "editor/text_window.h"
#include "editor/seeker.h"

#include "app/app.h"
#include "app/i18n_strings.h"
#include "app/skin.h"
#include "app/notebook.h"
#include "app/text_page.h"
#include "app/session.h"
#include "app/config.h"
#include "app/binding.h"
#include "app/active_list_window.h"

namespace jil {\

////////////////////////////////////////////////////////////////////////////////

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent) {
  editor::ErrorCode error_code = buffer->SaveFile();
  if (error_code == editor::kNoError) {
    return true;
  }

  // TODO: Detailed information.
  if (error_code == editor::kIOError) {
    // Read-only? No enough disk space?
  } else if (error_code == editor::kEncodingError) {
    // Failed in encoding conversion.
  }

  wxString msg = wxString::Format(_("Failed to save file! (%s)"), buffer->file_path_name());
  wxMessageBox(msg,
               _("Save File"),
               wxOK | wxCENTRE | wxICON_ERROR,
               parent);

  return false;
}

bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent) {
  // TODO: ext
  wxString file_path_name = wxSaveFileSelector(wxEmptyString, wxEmptyString);
  if (file_path_name.empty()) {
    // Save As is canceled.
    return false;
  }

  if (wxFileExists(file_path_name)) {
    // Confirm replace.
    wxString msg = wxString::Format(_("The file (%s) already exists. Replace it?"), file_path_name);
    int confirm_result = wxMessageBox(
        msg,
        _("Save File As"),
        wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_EXCLAMATION | wxCENTRE,
        parent);
    if (confirm_result == wxCANCEL) {
      // No replace.
      return false;
    }
  }

  buffer->set_file_path_name(file_path_name);

  return SaveBuffer(buffer, parent);
}

////////////////////////////////////////////////////////////////////////////////

// Drop files to book frame to open.
class FileDropTarget : public wxFileDropTarget {
public:
  FileDropTarget(BookFrame* book_frame)
      : main_frame_(book_frame) {
  }

  virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& file_names) OVERRIDE {
    main_frame_->OpenFiles(file_names);
    return true;
  }

private:
  BookFrame* main_frame_;
};

////////////////////////////////////////////////////////////////////////////////

// Char hook event is actually a key down event, so we give it a better name.
#define EVT_KEYDOWN_HOOK EVT_CHAR_HOOK

BEGIN_EVENT_TABLE(BookFrame, wxFrame)
EVT_ACTIVATE(BookFrame::OnActivate)
EVT_KEYDOWN_HOOK(BookFrame::OnKeyDownHook)

// File menu
EVT_MENU_RANGE(ID_MENU_FILE_BEGIN, ID_MENU_FILE_END - 1, BookFrame::OnMenuFile)
EVT_MENU(wxID_EXIT, BookFrame::OnQuit)

// Edit menu
EVT_MENU_RANGE(ID_MENU_EDIT_BEGIN, ID_MENU_EDIT_END - 1, BookFrame::OnMenuEdit)

// Tools menu
EVT_MENU(ID_MENU_TOOLS_OPTIONS, BookFrame::OnToolsOptions)

EVT_MENU(wxID_HELP, BookFrame::OnHelp)

// Update UI
EVT_UPDATE_UI_RANGE(ID_MENU_FILE_BEGIN, ID_MENU_FILE_END - 1, OnUpdateFileUI)
EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_BEGIN, ID_MENU_EDIT_END - 1, OnUpdateEditUI)

EVT_CLOSE(BookFrame::OnClose)

END_EVENT_TABLE()

BookFrame::BookFrame(Session* session)
    : session_(session)
    , text_binding_(NULL)
    , book_binding_(NULL)
    , active_list_window_(NULL) {
}

bool BookFrame::Create(wxWindow* parent, wxWindowID id, const wxString& title) {
  if (!wxFrame::Create(parent, id, title)) { //, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS)) {
    return false;
  }

  assert(theme_);
  assert(style_ != NULL);
  assert(text_binding_ != NULL);
  assert(book_binding_ != NULL);

  notebook_ = new Notebook(theme_->GetTheme(THEME_NOTEBOOK));
  notebook_->Create(this, ID_NOTEBOOK);

  LoadMenus();

  SetDropTarget(new FileDropTarget(this));

  return true;
}

BookFrame::~BookFrame() {
}

bool BookFrame::Show(bool show) {
  // Restore last state.
  wxRect rect = session_->book_frame_rect();
  if (rect.IsEmpty()) {
    wxClientDisplayRect(&rect.x, &rect.y, &rect.width, &rect.height);
    rect.Deflate(rect.width * 0.125, rect.height * 0.125);
  }
  SetSize(rect);

  if (session_->book_frame_maximized()) {
    Maximize();
  }

  return wxFrame::Show(show);
}

void BookFrame::OpenFile(const wxString& file_name, bool active) {
  using namespace editor;

  wxFileName fn_object(file_name);
  // Make the path absolute, resolve shortcut, etc.
  fn_object.Normalize(wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_SHORTCUT);

  // Check if this file has already been opened.
  TextPage* text_page = GetTextPage(fn_object);
  if (text_page != NULL) {
    if (active) {
      notebook_->ActivatePage(text_page);
    }
    return;
  }

  App& app = wxGetApp();

  wxString ext = fn_object.GetExt();
  FileType* file_type = app.GetFileType(ext);

  TextBuffer* buffer = TextBuffer::Create(fn_object, file_type, app.cjk_filters(), app.file_encoding());
  if (buffer == NULL) {
    wxMessageBox(wxString::Format(_("Failed to open file: %s"), fn_object.GetFullPath().c_str()));
    return;
  }

  notebook_->AddPage(CreateTextPage(buffer), active);
}

void BookFrame::OpenFiles(const wxArrayString& file_names) {
  const size_t size = file_names.size();
  if (size == 0) {
    return;
  }

  notebook_->StartBatch();

  // Open and activate the first file.
  OpenFile(file_names[0], /*active=*/true);

  // Open other files.
  for (size_t i = 1; i < file_names.size(); ++i) {
    OpenFile(file_names[i], /*active=*/false);
  }

  notebook_->EndBatch();
}

void BookFrame::FileNew() {
  using namespace editor;

  const wxChar* const kLangNull = wxEmptyString;
  FileType* file_type = wxGetApp().GetFileType(kLangNull);

  TextBuffer* buffer = TextBuffer::Create(file_type, wxGetApp().file_encoding());

  TextPage* text_page = CreateTextPage(buffer);
  notebook_->AddPage(text_page, true);
}

void BookFrame::FileNewFromStub() {
  using namespace editor;

  wxString stub_dir = wxStandardPaths::Get().GetPluginsDir() + wxFILE_SEP_PATH + wxT("stub");

  //wxDir dir_obj(stub_dir);
  //if (!dir_obj.IsOpened()) {
  //  return;
  //}

  //wxString filename;
  //bool cont = dir_obj.GetFirst(&filename, wxT("*.stub"), wxDIR_FILES);
  //while (cont) {
  //  cont = dir_obj.GetNext(&filename);
  //}

  wxFileDialog file_dialog(this,
                           wxFileSelectorPromptStr,
                           wxEmptyString,
                           wxEmptyString,
                           wxFileSelectorDefaultWildcardStr,
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
  if (file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  wxString stub_path = file_dialog.GetPath();

  App& app = wxGetApp();

  const wxChar* const kLangNull = wxEmptyString;
  FileType* file_type = app.GetFileType(kLangNull);

  wxFileName fn_stub(stub_path);
  TextBuffer* buffer = TextBuffer::Create(fn_stub, file_type, app.cjk_filters(), app.file_encoding());
  // Clear the stub file name since this is a new file.
  buffer->set_file_name_object(wxFileName());

  TextPage* text_page = CreateTextPage(buffer);
  notebook_->AddPage(text_page, true);
}

void BookFrame::FileNewWindow() {
  BookFrame* book_frame = new BookFrame(session_);
  book_frame->set_theme(theme_);
  book_frame->set_style(style_);
  book_frame->set_text_binding(text_binding_);
  book_frame->set_book_binding(book_binding_);

  if (!book_frame->Create(NULL, wxID_ANY, kAppDisplayName)) {
    wxLogError(wxT("Failed to create book frame!"));
    return;
  }
  book_frame->Show();
}

void BookFrame::FileOpen() {
  wxFileDialog file_dialog(this,
                           wxFileSelectorPromptStr,
                           wxEmptyString,
                           wxEmptyString,
                           wxFileSelectorDefaultWildcardStr,
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR | wxFD_PREVIEW);
  if (file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  wxArrayString file_names;
  // Note: GetFilenames() returns file names instead of full path names.
  // Without wxFD_CHANGE_DIR, OpenFiles will be failed.
  file_dialog.GetFilenames(file_names);
  if (file_names.empty()) {
    return;
  }

  OpenFiles(file_names);
}

void BookFrame::FileClose() {
  notebook_->RemoveActivePage();
}

void BookFrame::FileCloseAll() {
  notebook_->RemoveAllPages();
}

void BookFrame::FileSave() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    DoSaveBuffer(buffer);
  }
}

void BookFrame::FileSaveAs() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    SaveBufferAs(buffer, this);
  }
}

void BookFrame::FileSaveAll() {
  // TODO
  std::vector<editor::notebook::IPage*> pages = notebook_->GetPages();
  foreach (editor::notebook::IPage* page, pages) {
    TextPage* text_page = wxDynamicCast(page->NPage_AsWindow(), TextPage);
    if (text_page != NULL) {
      DoSaveBuffer(text_page->buffer());
    }
  }
}

//------------------------------------------------------------------------------

void BookFrame::SwitchPage() {
  //if (active_list_window_ == NULL) {
  //  active_list_window_ = new ActiveListWindow(this, wxID_ANY);
  //}

  //std::vector<wxString> active_files;
  //active_files.push_back(wxT("notebook.h"));
  //active_files.push_back(wxT("notebook.cc"));
  //active_files.push_back(wxT("text_window.cc"));
  //active_files.push_back(wxT("app.cc"));
  //active_files.push_back(wxT("text_window.h"));
  //active_list_window_->set_files(active_files);
  //active_list_window_->Show();

  notebook_->SwitchPage();
}

//------------------------------------------------------------------------------

// Set current active book frame as the top window.
// TODO: Match command func to execute for commands not in the menu.
void BookFrame::OnActivate(wxActivateEvent& evt) {
  if (evt.GetActive()) {
    if (this != wxGetApp().GetTopWindow()) {
      wxGetApp().SetTopWindow(this);
    }
  }
  evt.Skip();
}

void BookFrame::OnKeyDownHook(wxKeyEvent& evt) {
  // Skip for accelerators and child windows, e.g., TextWindow.
  evt.Skip();

  const int code = evt.GetKeyCode();

  if (code == WXK_NONE) {
    return;
  }
  if (code == WXK_CONTROL || code == WXK_SHIFT || code == WXK_ALT) {
    return;
  }

  if (code == WXK_ESCAPE) {
    if (!down_key_.leader().IsEmpty()) {
      down_key_.Reset();
      return;
    }
  }

  const int modifiers = evt.GetModifiers() ;

  if (down_key_.leader().IsEmpty() && modifiers != 0) { // Leader key always has modifiers.
    if (book_binding_->IsLeaderKey(code, modifiers)) {
      down_key_.set_leader(code, modifiers);
      return;
    }
  }

  down_key_.Set(down_key_.leader_code(), down_key_.leader_modifiers(), code, modifiers); // TODO
  if (!book_binding_->IsMenuKey(down_key_)) {
    down_key_.Reset();
  }
}

void BookFrame::OnMenuFile(wxCommandEvent& evt) {
  int menu_id = evt.GetId();

  if (!down_key_.IsEmpty()) {
    if (!CheckLeaderKey(menu_id)) {
    return;
    }
  }

  switch (menu_id) {
    case ID_MENU_FILE_NEW:
      FileNew();
      break;

    case ID_MENU_FILE_NEW_FROM_STUB:
      FileNewFromStub();
      break;

    case ID_MENU_FILE_NEW_WINDOW:
      FileNewWindow();
      break;

    case ID_MENU_FILE_OPEN:
      FileOpen();
      break;

    case ID_MENU_FILE_CLOSE:
      FileClose();
      break;

    case ID_MENU_FILE_CLOSE_ALL:
      FileCloseAll();
      break;

    case ID_MENU_FILE_SAVE:
      FileSave();
      break;

    case ID_MENU_FILE_SAVE_AS:
      FileSaveAs();
      break;

    case ID_MENU_FILE_SAVE_ALL:
      FileSaveAll();
      break;
  }
}

void BookFrame::OnQuit(wxCommandEvent& WXUNUSED(evt)) {
  if (!notebook_->RemoveAllPages()) {
    return;
  }
  Close(true);
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuEdit(wxCommandEvent& evt) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    // Edit menu commands all need text window.
    return;
  }

  int menu_id = evt.GetId();

#ifdef __WXDEBUG__
  if (menu_id == ID_MENU_LOG_LINE_LENGTH_TABLE) {
    text_page->buffer()->LogLineLengthTable();
  }
#endif // __WXDEBUG__

  if (!down_key_.IsEmpty()) {
    if (!CheckLeaderKey(menu_id)) {
      return;
    }
  }

  editor::TextFunc text_func = book_binding_->GetTextFunc(menu_id);
  if (text_func == NULL) {
    wxLogError("BookFrame::OnMenuEdit - No command function found for menu %d!", menu_id);
    return;
  }

  text_func(text_page);
}

//------------------------------------------------------------------------------

void BookFrame::OnToolsOptions(wxCommandEvent& WXUNUSED(evt)) {
  wxMessageBox(_T("Options..."));
}

//------------------------------------------------------------------------------

void BookFrame::OnHelp(wxCommandEvent& evt) {
  wxMessageBox(_T("Help..."));
}

//------------------------------------------------------------------------------

// Update the state and, if necessary, the label of menu items.
void BookFrame::OnUpdateFileUI(wxUpdateUIEvent& evt) {
  using namespace editor;
  TextPage* text_page = ActiveTextPage();
  TextBuffer* buffer = text_page == NULL ? NULL : text_page->buffer();

  switch (evt.GetId()) {
  case ID_MENU_FILE_CLOSE:
    {
      //wxString text = buffer == NULL ? kTrFileClose : wxString::Format(kTrFileCloseFormat, text_page->NPage_GetLabel().c_str());
      //evt.SetText(AppendAccel(text, kAccelFileClose));
      evt.Enable(text_page != NULL);
    }
    break;

  case ID_MENU_FILE_CLOSE_ALL:
    evt.Enable(text_page != NULL);
    break;

  case ID_MENU_FILE_SAVE:
    {
      //wxString text = buffer == NULL ? kTrFileSave : wxString::Format(kTrFileSaveFormat, text_page->NPage_GetLabel().c_str());
      //evt.SetText(AppendAccel(text, kAccelFileSave));
      evt.Enable(buffer != NULL && buffer->modified());
    }
    break;

  case ID_MENU_FILE_SAVE_AS:
    {
      //wxString text = buffer == NULL ? kTrFileSaveAs : wxString::Format(kTrFileSaveAsFormat, text_page->NPage_GetLabel().c_str());
      //evt.SetText(text);
      evt.Enable(buffer != NULL);
    }
    break;

  case ID_MENU_FILE_SAVE_ALL:
    evt.Enable(buffer != NULL);
    break;

  default:
    break;
  }
}

void BookFrame::OnUpdateEditUI(wxUpdateUIEvent& evt) {
  TextPage* text_page = ActiveTextPage();

  switch (evt.GetId()) {
    case ID_MENU_EDIT_UNDO:
      evt.Enable(text_page != NULL && text_page->CanUndo());
      break;

    case ID_MENU_EDIT_REDO:
      evt.Enable(text_page != NULL && (text_page->CanRedo() || text_page->CanRepeat()));
      break;

    case ID_MENU_EDIT_COPY:
      evt.Enable(text_page != NULL);
      break;

    case ID_MENU_EDIT_PASTE: // TODO
      evt.Enable(text_page != NULL);
      break;

    case ID_MENU_EDIT_INSERT_FILE:
      evt.Enable(text_page != NULL);
      break;

    case ID_MENU_EDIT_WRAP:
      evt.Check(text_page == NULL ? false : text_page->wrapped());
      evt.Enable(text_page != NULL);
      break;

    default:
      break;
  }
}

void BookFrame::OnClose(wxCloseEvent& evt) {
  if (!notebook_->RemoveAllPages()) {
    return;
  }

  if (IsMaximized()) {
    session_->set_book_frame_maximized(true);
  } else {
    session_->set_book_frame_maximized(false);
    session_->set_book_frame_rect(GetScreenRect());
  }

  evt.Skip();
}

//------------------------------------------------------------------------------

void BookFrame::NewMenuItem(wxMenu* menu, int id, const wxString& label, wxItemKind item_kind) {
  wxString menu_label = label;

  // Append accelerator if any.
  editor::Key menu_key = book_binding_->GetMenuKey(id);
  if (!menu_key.IsEmpty()) {
    menu_label += wxT("\t") + menu_key.ToString();
  }

  menu->Append(new wxMenuItem(menu, id, menu_label, wxEmptyString, item_kind));
}

void BookFrame::LoadMenus() {
  wxMenuBar* menu_bar = new wxMenuBar;

  //////////////////////////////////////
  // File
  wxMenu* menu_file = new wxMenu;
  // - New
  NewMenuItem(menu_file, ID_MENU_FILE_NEW, kTrFileNew);
  NewMenuItem(menu_file, ID_MENU_FILE_NEW_FROM_STUB, kTrFileNewFromStub);
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_NEW_WINDOW, kTrFileNewFrame);

  // - Open
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_OPEN, kTrFileOpen);
  menu_file->AppendSeparator();
  // - Close
  NewMenuItem(menu_file, ID_MENU_FILE_CLOSE, kTrFileClose);
  NewMenuItem(menu_file, ID_MENU_FILE_CLOSE_ALL, kTrFileCloseAll);
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE, kTrFileSave);
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE_AS, kTrFileSaveAs);
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE_ALL, kTrFileSaveAll);
  menu_file->AppendSeparator();
  // - Exit
  NewMenuItem(menu_file, wxID_EXIT, kTrFileExit); // TODO: ID

  menu_bar->Append(menu_file, _T("&File"));

  //////////////////////////////////////
  // Edit
  wxMenu* menu_edit = new wxMenu;
  NewMenuItem(menu_edit, ID_MENU_EDIT_UNDO, kTrEditUndo);
  NewMenuItem(menu_edit, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu_edit->AppendSeparator();
  NewMenuItem(menu_edit, ID_MENU_EDIT_CUT, kTrEditCut);
  NewMenuItem(menu_edit, ID_MENU_EDIT_COPY, kTrEditCopy);
  NewMenuItem(menu_edit, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu_edit->AppendSeparator();
  NewMenuItem(menu_edit, ID_MENU_EDIT_INSERT_FILE, kTrEditInsertFile);
  NewMenuItem(menu_edit, ID_MENU_EDIT_WRAP, kTrEditWrap, wxITEM_CHECK);
  menu_edit->AppendSeparator();
#ifdef __WXDEBUG__
  NewMenuItem(menu_edit, ID_MENU_LOG_LINE_LENGTH_TABLE, wxT("[DEBUG] Log Line Length Table"));
#endif
  menu_bar->Append(menu_edit, _T("&Edit"));

  //////////////////////////////////////
  // Tools
  /*
  wxMenu* menu_tools = new wxMenu;
  menu_tools->Append(ID_MENU_TOOLS_OPTIONS, _T("&Options..."));
  menu_bar->Append(menu_tools, _T("&Tools"));
  */

  SetMenuBar(menu_bar);
}

bool BookFrame::CheckLeaderKey(int menu_id) {
  editor::Key menu_key = book_binding_->GetMenuKey(menu_id);
  if (menu_key.IsEmpty()) {
    // This menu item has no shortcut.
    return true;
  }

  bool ok = menu_key.leader() == down_key_.leader();

  if (!down_key_.leader().IsEmpty()) {
    // Clear current text window's leader key.
    TextPage* page = ActiveTextPage();
    if (page != NULL) {
      page->set_leader_key();
    }
  }
  // Clear down key.
  down_key_.Reset();

  return ok;
}

TextPage* BookFrame::CreateTextPage(editor::TextBuffer* buffer) {
  TextPage* text_page = new TextPage(buffer);

  text_page->set_style(style_);
  text_page->set_theme(theme_->GetTheme(THEME_TEXT_WINDOW));
  text_page->set_binding(text_binding_);

  text_page->Create(notebook_, wxID_ANY);

  // Hide to avoid flicker. Will be shown when it's activated.
  text_page->Hide();

  text_page->SetTextFont(wxGetApp().font());
  text_page->SetLineNrFont(wxGetApp().font());

  text_page->SetStatusFields(wxGetApp().status_fields());

  return text_page;
}

TextPage* BookFrame::ActiveTextPage() const {
  editor::notebook::IPage* page = notebook_->GetActivePage();
  if (page == NULL) {
    return NULL;
  }
  return wxDynamicCast(page->NPage_AsWindow(), TextPage);
}

editor::TextBuffer* BookFrame::ActiveBuffer() const {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return NULL;
  }
  return text_page->buffer();
}

TextPage* BookFrame::GetTextPage(const wxFileName& fn_object) const {
  std::vector<editor::notebook::IPage*> pages = notebook_->GetPages();
  foreach (editor::notebook::IPage* page, pages) {
    TextPage* text_page = wxDynamicCast(page->NPage_AsWindow(), TextPage);
    if (text_page != NULL) {
      if (fn_object == text_page->buffer()->file_name_object()) {
        return text_page;
      }
    }
  }

  return NULL;
}

void BookFrame::DoSaveBuffer(editor::TextBuffer* buffer) {
  assert(buffer != NULL);

  if (!buffer->modified()) {
    return;
  }

  if (buffer->new_created() || buffer->read_only()) {
    SaveBufferAs(buffer, this);
  } else {
    SaveBuffer(buffer, this);
  }
}

} // namespace jil
