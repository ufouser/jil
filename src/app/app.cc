#include "app/app.h"

#include "boost/algorithm/string.hpp"

#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"

#include "wx/image.h"
#include "wx/intl.h"
#include "wx/sysopt.h"
#include "wx/stdpaths.h"
#include "wx/filename.h"
#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/snglinst.h"
#include "wx/debugrpt.h"
#include "wx/log.h"
#include "wx/msgdlg.h"

#include "base/string_util.h"

#include "editor/notebook.h"
#include "editor/text_window.h"
#include "editor/file_type.h"
#include "editor/style.h"
#include "editor/color.h"

#include "app/font_util.h"
#include "app/binding_config.h"
#include "app/book_frame.h"
#include "app/binding.h"

#define kConfigFile wxT("options.cfg")
#define kSessionFile wxT("session.cfg")

IMPLEMENT_WXWIN_MAIN

namespace jil {\

////////////////////////////////////////////////////////////////////////////////

class Connection : public wxConnection {
public:
  virtual bool OnExec(const wxString& topic, const wxString& data) OVERRIDE {
    BookFrame* book_frame = wxDynamicCast(wxGetApp().GetTopWindow(), BookFrame);
    if (book_frame == NULL) {
      return false;
    }

    // Raise the book frame.
    if (book_frame->IsIconized()) {
      book_frame->Iconize(false);
    }
    book_frame->Raise();

    // Open the files.
    if (!data.IsEmpty()) {
      book_frame->OpenFiles(wxSplit(data, wxT(';')));
    }

    return true;
  }
};

// The 1st instance will create a server for IPC.
class Server : public wxServer {
public:
  virtual wxConnectionBase* OnAcceptConnection(const wxString& topic) OVERRIDE {
    if (topic.Lower() != kAppName) {
      return NULL;
    }

    // Check that there are no modal dialogs active.
    wxWindowList::Node* node = wxTopLevelWindows.GetFirst();
    while (node != NULL) {
      wxDialog* dialog = wxDynamicCast(node->GetData(), wxDialog);
      if (dialog != NULL && dialog->IsModal()) {
        return NULL;
      }
      node = node->GetNext();
    }

    return new Connection;
  }
};

// The 2nd instance communicates with the 1st instance as a client.
class Client : public wxClient {
public:
  virtual wxConnectionBase* OnMakeConnection() OVERRIDE {
    return new Connection;
  }
};

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP_NO_MAIN(App)

App::App()
    : instance_checker_(NULL)
    , server_(NULL)
    , cjk_filters_(0)
    , style_(new editor::Style)
    , text_binding_(new editor::Binding)
    , book_binding_(new BookBinding) {
}

App::~App() {
  delete style_;
  delete text_binding_;
  delete book_binding_;

  for (size_t i = 0; i < file_types_.size(); ++i) {
    delete file_types_[i];
  }
  file_types_.clear();

  wxDELETE(server_);
}

bool App::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

  // Single instance check and the communication between two instances.

  const wxString instance_name = wxString::Format(wxT("%s-%s"), kAppName, wxGetUserId().c_str());
  instance_checker_ = new wxSingleInstanceChecker(instance_name);

  if (!instance_checker_->IsAnotherRunning()) {
    server_ = new Server;
    if (!server_->Create(kAppName)) {
      wxLogError(wxT("Failed to create an IPC service."));
    }
  } else {
    // Another instance is running.
    // Connect to it and send it any file name before exiting.
    Client* client = new Client;

    // TODO: 2nd parameter
    wxConnectionBase* connection = client->MakeConnection(wxT("localhost"), kAppName, kAppName);
    if (connection != NULL) {
      // Ask the other instance to open a file or raise itself.
      connection->Execute(wxJoin(cmdline_files_, wxT(';')));
      connection->Disconnect();
      delete connection;
    } else {
      wxLogWarning(wxT("Failed to connect to the existing instance. Any modal dialogs opened?"));
    }

    delete client;
    return false;
  }

  SetAppName(kAppName);
  SetAppDisplayName(kAppDisplayName);

  wxImage::AddHandler(new wxPNGHandler);

#ifdef __WXMSW__
  // Switched off the colour remapping and use a transparent background for toolbar bitmaps.
  if (wxTheApp->GetComCtl32Version() >= 600 && ::wxDisplayDepth() >= 32) {
    wxSystemOptions::SetOption(_T("msw.remap"), 2);
  }
#endif // __WXMSW__

  // Make sure the user data dir exists.
  wxString user_data_dir = GetUserDataDir();
  if (!wxDir::Exists(user_data_dir)) {
    wxMkdir(user_data_dir);
  }

  LoadOptions();

  if (!LoadStatusFields()) {
    UseDefaultStatusFields();
  }

  if (!LoadTheme()) {
    return false;
  }

  LoadStyle();

  LoadBinding();

  // Load session.
  session_.Load(user_data_dir + kSessionFile);

  // Create book frame.
  BookFrame* book_frame = new BookFrame(&session_);
  book_frame->set_theme(theme_);
  book_frame->set_style(style_);
  book_frame->set_text_binding(text_binding_);
  book_frame->set_book_binding(book_binding_);

  if (!book_frame->Create(NULL, wxID_ANY, kAppDisplayName)) {
    wxLogError(wxT("Failed to create book frame!"));
    return false;
  }

  SetTopWindow(book_frame);
  book_frame->Show();

  // Open the files specified as command line arguments.
  if (!cmdline_files_.empty()) {
    book_frame->OpenFiles(cmdline_files_);
    cmdline_files_.clear();
  }

  return true;
}

int App::OnExit() {
  session_.Save(GetUserDataDir() + kSessionFile);
  return wxApp::OnExit();
}

editor::FileType* App::GetFileType(const wxString& ft_name) {
  using namespace editor;

  for (size_t i = 0; i < file_types_.size(); ++i) {
    if (ft_name == file_types_[i]->name()) {
      return file_types_[i];
    }
  }

  FileType* file_type = new FileType(ft_name);

  file_type->Load();

  // Load file type specific options.
  editor::Options& options = file_type->options();
  options = options_; // Firstly, copy global options.

  wxString file_type_dir = GetUserDataDir() + wxT("ftplugin") + wxFILE_SEP_PATH + file_type->name() + wxFILE_SEP_PATH;
  Config options_config;
  if (options_config.Load(file_type_dir + wxT("options.cfg"))) {
    LoadEditOptions(&options_config, options);
  }

  file_types_.push_back(file_type);
  return file_type;
}

// Generate debug report on fatal exception.
void App::OnFatalException() {
  wxDebugReportCompress debug_report;
  debug_report.AddAll(wxDebugReport::Context_Exception);

  if (debug_report.Process()) {
    wxString report_file_name = debug_report.GetCompressedFileName();
  }
}

// Command line parsing.

#if wxUSE_CMDLINE_PARSER

void App::OnInitCmdLine(wxCmdLineParser& parser) {
  wxApp::OnInitCmdLine(parser);
  parser.AddParam(_("file"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL);
}

bool App::OnCmdLineParsed(wxCmdLineParser& parser) {
  wxApp::OnCmdLineParsed(parser);

  size_t param_count = parser.GetParamCount();
  for (size_t i = 0; i < param_count; ++i) {
    cmdline_files_.push_back(parser.GetParam(i));
  }

  return true;
}

bool App::OnCmdLineHelp(wxCmdLineParser& parser) {
  // Print usage and exit (by returning false).
  parser.Usage();
  return false;
}

bool App::OnCmdLineError(wxCmdLineParser& parser) {
  return true; // TODO
}

#endif // wxUSE_CMDLINE_PARSER

const wxString& App::GetPluginsDir() const {
  static wxString plugins_dir = wxStandardPaths::Get().GetPluginsDir() + wxFILE_SEP_PATH;
  return plugins_dir;
}

const wxString& App::GetUserDataDir() const {
  static wxString user_data_dir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH;
  return user_data_dir;
}

void App::SetCjkFilters(const std::string& cjk) {
  cjk_filters_ = 0;

  if (!cjk.empty()) {
    std::vector<std::string> cjk_values;
    boost::split(cjk_values, cjk, boost::is_any_of(", "), boost::token_compress_on);

    for (size_t i = 0; i < cjk_values.size(); ++i) {
      if (cjk_values[i] == "cs") {
        cjk_filters_ |= NS_FILTER_CHINESE_SIMPLIFIED;
      } else if (cjk_values[i] == "ct") {
        cjk_filters_ |= NS_FILTER_CHINESE_TRADITIONAL;
      } else if (cjk_values[i] == "c") {
        cjk_filters_ |= (NS_FILTER_CHINESE_SIMPLIFIED | NS_FILTER_CHINESE_TRADITIONAL);
      } else if (cjk_values[i] == "j") {
        cjk_filters_ |= NS_FILTER_JAPANESE;
      } else if (cjk_values[i] == "k") {
        cjk_filters_ |= NS_FILTER_KOREAN;
      }
    }
  }

  wxLocale locale;
  locale.Init();
  int lang = locale.GetLanguage();

  if ((cjk_filters_ & NS_FILTER_CHINESE_SIMPLIFIED) == 0) {
    if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED) {
      cjk_filters_ |= NS_FILTER_CHINESE_SIMPLIFIED;
    }
  }
  if ((cjk_filters_ & NS_FILTER_CHINESE_TRADITIONAL) == 0) {
    if (lang == wxLANGUAGE_CHINESE ||
        lang == wxLANGUAGE_CHINESE_TRADITIONAL ||
        lang == wxLANGUAGE_CHINESE_HONGKONG ||
        lang == wxLANGUAGE_CHINESE_MACAU ||
        lang == wxLANGUAGE_CHINESE_SINGAPORE ||
        lang == wxLANGUAGE_CHINESE_TAIWAN) {
      cjk_filters_ |= NS_FILTER_CHINESE_TRADITIONAL;
    }
  }

  if ((cjk_filters_ & NS_FILTER_JAPANESE) == 0 && lang == wxLANGUAGE_JAPANESE) {
    cjk_filters_ |= NS_FILTER_JAPANESE;
  }

  if ((cjk_filters_ & NS_FILTER_KOREAN) == 0 && lang == wxLANGUAGE_KOREAN) {
    cjk_filters_ |= NS_FILTER_KOREAN;
  }
}

void App::LoadOptions() {
  Config config;
  config.Load(GetUserDataDir() + kConfigFile);

  Setting root_setting = config.root();

  std::string cjk = root_setting.GetString(CJK);
  SetCjkFilters(cjk);

  file_encoding_ = wxString::FromAscii(root_setting.GetString(FILE_ENCODING));
  if (file_encoding_.empty()) {
    file_encoding_ = editor::UTF_8;
  }

  font_ = root_setting.GetFont(FONT, 10, GetDefaultFont());

  theme_name_ = root_setting.GetString(THEME);

  LoadEditOptions(&config, options_);
}

void App::LoadEditOptions(Config* config, editor::Options& options) {
  Setting root_setting = config->root();

  Setting setting = root_setting.Get(WRAP);
  if (setting && setting.type() == Setting::kBool) {
    options.wrap = setting.GetBool();
  }

  setting = root_setting.Get(EXPAND_TAB);
  if (setting && setting.type() == Setting::kBool) {
    options.expand_tab = setting.GetBool();
  }

  setting = root_setting.Get(TAB_STOP);
  if (setting && setting.type() == Setting::kInt) {
    options.tab_stop = setting.GetInt();
  }

  setting = root_setting.Get(SHOW_NUMBER);
  if (setting && setting.type() == Setting::kBool) {
    options.show_number = setting.GetBool();
  }

  setting = root_setting.Get(SHOW_SPACE);
  if (setting && setting.type() == Setting::kBool) {
    options.show_space = setting.GetBool();
  }

  setting = root_setting.Get(LINE_HIGHLIGHT);
  if (setting && setting.type() == Setting::kInt) {
    options.line_highlight = setting.GetInt();
  }
}

wxString App::GetThemeFile() {
  wxString theme_subdir(wxT("theme"));
  theme_subdir += wxFILE_SEP_PATH;

  if (theme_name_.empty()) {
    return GetPluginsDir() + theme_subdir + wxT("default.cfg");
  }

  wxString theme_file_name = theme_name_ + wxT(".cfg");

  wxString theme_file = GetPluginsDir() + theme_subdir + theme_file_name;
  if (wxFileName::Exists(theme_file)) {
    return theme_file;
  }

  theme_file = GetUserDataDir() + theme_subdir + theme_file_name;
  if (wxFileName::Exists(theme_file)) {
    return theme_file;
  }

  return GetPluginsDir() + theme_subdir + wxT("default.cfg");
}

bool App::LoadThemeFile(const wxString& file) {
  using editor::SharedTheme;
  using editor::Theme;
  using editor::TextWindow;
  using editor::StatusLine;

  Config config;
  if (!config.Load(file)) {
    return false;
  }

  // Text window.
  SharedTheme tw_theme(new Theme(TextWindow::THEME_COUNT, TextWindow::COLOR_COUNT, 0));

  Setting tw_setting = config.root().Get("text_window");
  if (tw_setting) {
    tw_theme->SetColor(TextWindow::TEXT_BG, tw_setting.GetColor("text_bg"));

    // Status line.
    SharedTheme sl_theme(new Theme(StatusLine::COLOR_COUNT, StatusLine::FONT_COUNT));

    Setting sl_setting = tw_setting.Get("status_line");
    if (sl_setting) {
      sl_theme->SetColor(StatusLine::BORDER, sl_setting.GetColor("border"));
      sl_theme->SetColor(StatusLine::FG, sl_setting.GetColor("fg"));
      sl_theme->SetColor(StatusLine::BG, sl_setting.GetColor("bg"));
      sl_theme->SetFont(StatusLine::FONT, sl_setting.GetFont("font"));
    }

    tw_theme->SetTheme(TextWindow::THEME_STATUS_LINE, sl_theme);
  }

  theme_->SetTheme(BookFrame::THEME_TEXT_WINDOW, tw_theme);

  // Notebook.
  SharedTheme nb_theme(new Theme(editor::Notebook::COLOR_COUNT, editor::Notebook::FONT_COUNT));

  Setting nb_setting = config.root().Get("notebook");
  if (nb_setting) {
    nb_theme->SetColor(editor::Notebook::BG, nb_setting.GetColor("bg"));
    nb_theme->SetColor(editor::Notebook::TAB_AREA_BG, nb_setting.GetColor("tab_area_bg"));
    nb_theme->SetColor(editor::Notebook::TAB_FG, nb_setting.GetColor("tab_fg"));
    nb_theme->SetColor(editor::Notebook::ACTIVE_TAB_FG, nb_setting.GetColor("active_tab_fg"));
    nb_theme->SetColor(editor::Notebook::TAB_BG, nb_setting.GetColor("tab_bg"));
    nb_theme->SetColor(editor::Notebook::ACTIVE_TAB_BG, nb_setting.GetColor("active_tab_bg"));
    nb_theme->SetColor(editor::Notebook::TAB_BORDER, nb_setting.GetColor("tab_border"));
    nb_theme->SetColor(editor::Notebook::ACTIVE_TAB_BORDER, nb_setting.GetColor("active_tab_border"));

    nb_theme->SetFont(editor::Notebook::TAB_FONT, nb_setting.GetFont("tab_font"));
  }

  theme_->SetTheme(BookFrame::THEME_NOTEBOOK, nb_theme);

  return true;
}

bool App::LoadTheme() {
  theme_.reset(new editor::Theme(BookFrame::THEME_COUNT));

  if (!LoadThemeFile(GetThemeFile())) {
    wxMessageBox(_("Failed to load theme. Please reinstall to fix the issue."));
    return false;
  }

  return true;
}

static editor::Style::Item* ReadStyleItem(Config& style_config,
                                          const std::string& item_name,
                                          const wxColour& default_fg,
                                          const wxColour& default_bg) {
  Setting item_setting = style_config.Lookup(item_name.c_str());
  if (!item_setting) {
    return new editor::Style::Item(item_name, default_fg, default_bg);
  }
  wxColour fg = item_setting.GetColor("fg", default_fg);
  wxColour bg = item_setting.GetColor("bg", default_bg);
  return new editor::Style::Item(item_name, fg, bg);
}

// TODO
void App::LoadStyle() {
  using namespace editor;

  wxString style_file = GetPluginsDir() + wxT("style") + wxFILE_SEP_PATH + wxT("default.cfg");

  Config config;
  if (!config.Load(style_file)) {
    // TODO
  }

  wxColour dark_grey = Color::Get(Color::DARK_GREY);
  wxColour grey = Color::Get(Color::GREY);

  style_->Add(ReadStyleItem(config, Style::kNormal, *wxWHITE, dark_grey));
  style_->Add(ReadStyleItem(config, Style::kVisual, *wxWHITE, Color::Get(Color::GOLD)));
  style_->Add(ReadStyleItem(config, Style::kNumber, Color::Get(Color::GOLD), dark_grey));

  wxColour caret_line_bg = IncColor(dark_grey, 20);
  style_->Add(new Style::Item(Style::kCaretLine, *wxBLACK, caret_line_bg));
  style_->Add(new Style::Item(Style::kCaret, *wxBLACK, dark_grey));
  style_->Add(new Style::Item(Style::kSpace, *wxRED, dark_grey));

  /*
  style->Add(new Style::Item(lex::Group::kComment, *wxGREEN, normal_bg));
  style->Add(new Style::Item(lex::Group::kOperator, *wxBLACK, normal_bg));
  style->Add(new Style::Item(lex::Group::kConstant, *wxCYAN, normal_bg));
  style->Add(new Style::Item(lex::Group::kIdentifier, *wxBLACK, normal_bg));
  style->Add(new Style::Item(lex::Group::kStatement, *wxBLUE, normal_bg));
  style->Add(new Style::Item(lex::Group::kPackage, *wxBLUE, normal_bg));
  style->Add(new Style::Item(lex::Group::kPreProc, *wxBLACK, normal_bg));
  style->Add(new Style::Item(lex::Group::kType, *wxBLUE, normal_bg));
  style->Add(new Style::Item(lex::Group::kSpecial, *wxCYAN, normal_bg));
  style->Add(new Style::Item(lex::Group::kError, *wxRED, normal_bg));
  style->Add(new Style::Item(lex::Group::kTodo, *wxRED, normal_bg));
  */
}

bool App::LoadBinding() {
  // Always load default binding.
  BindingConfig binding_config(text_binding_, book_binding_);

  if (!binding_config.Load(GetPluginsDir() + wxT("binding.cfg"))) {
    return false;
  }

  binding_config.Load(GetUserDataDir() + wxT("binding.cfg"));
}

static editor::StatusLine::FieldId ParseFieldId(const std::string& field_id_str) {
  using namespace editor;

  static const std::string kFieldIds[StatusLine::kField_Count] = {
    "cwd",
    "encoding",
    "file_format",
    "file_name",
    "file_path",
    "file_type",
    "caret",
    "key_stroke",
    "space",
  };

  for (size_t i = 0; i < StatusLine::kField_Count; ++i) {
    if (field_id_str == kFieldIds[i]) {
      return static_cast<StatusLine::FieldId>(i);
    }
  }

  return StatusLine::kField_Count;
}

static wxAlignment ParseFieldAlign(const std::string& align_str) {
  if (align_str == "left") {
    return wxALIGN_LEFT;
  } else if (align_str == "right") {
    return wxALIGN_RIGHT;
  } else if (align_str == "center") {
    return wxALIGN_CENTER_HORIZONTAL;
  } else {
    return wxALIGN_NOT;
  }
}

static bool ParseFieldSizeType(const std::string& size_type_str,
                               editor::StatusLine::SizeType& size_type) {
  using namespace editor;

  if (size_type_str == "fit") {
    size_type = StatusLine::kFit;
  } else if (size_type_str == "stretch") {
    size_type = StatusLine::kStretch;
  } else if (size_type_str == "fixed_pixel") {
    size_type = StatusLine::kFixedPixel;
  } else if (size_type_str == "fixed_percentage") {
    size_type = StatusLine::kFixedPercentage;
  } else {
    return false;
  }

  return true;
}

static bool ParseFieldInfo(std::string& field_info_str, editor::StatusLine::FieldInfo& field_info) {
  using namespace editor;

  std::vector<std::string> field_strs;
  boost::split(field_strs, field_info_str, boost::is_any_of(","), boost::token_compress_off);
  if (field_strs.size() != 4) {
    wxLogDebug(wxT("Invalid status field: %s"), field_info_str.c_str());
    return false;
  }

  field_info.field_id = ParseFieldId(field_strs[0]);
  if (field_info.field_id == StatusLine::kField_Count) {
    return false;
  }

  field_info.align = ParseFieldAlign(field_strs[1]);

  if (!ParseFieldSizeType(field_strs[2], field_info.size_type)) {
    return false;
  }

  field_info.size_value = base::LexicalCast<int>(field_strs[3], 0);
  if (field_info.size_value < 0) {
    return false;
  }
  if (field_info.size_value == 0) {
    if (field_info.size_value == StatusLine::kFixedPixel ||
      field_info.size_value == StatusLine::kFixedPercentage) {
        return false;
    }
  }

  return true;
}

bool App::LoadStatusFields() {
  wxString status_line_file = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("status_line.cfg");
  Config config;
  if (!config.Load(status_line_file)) {
    return false;
  }

  Setting fields_setting = config.root().Get("fields");
  if (!fields_setting || fields_setting.type() != Setting::kArray) {
    return false;
  }

  editor::StatusLine::FieldInfo field_info;

  const int length = fields_setting.size();
  for (int i = 0; i < length; ++i) {
    Setting field_setting = fields_setting.Get(i);
    if (!field_setting || field_setting.type() != Setting::kString) {
      break;
    }

    std::string field_info_str = field_setting.GetString();
    if (ParseFieldInfo(field_info_str, field_info)) {
      status_fields_.push_back(field_info);
    }
  }

  return true;
}

static void SetFieldInfo(editor::StatusLine::FieldInfo& field_info,
                         editor::StatusLine::FieldId field_id,
                         wxAlignment align,
                         editor::StatusLine::SizeType size_type,
                         int size_value) {
  field_info.field_id = field_id;
  field_info.align = align;
  field_info.size_type = size_type;
  field_info.size_value = size_value;
  field_info.size = 0;
}

void App::UseDefaultStatusFields() {
  using namespace editor;

  status_fields_.resize(8);
  size_t i = 0;
  SetFieldInfo(status_fields_[i++], StatusLine::kField_Caret, wxALIGN_LEFT, StatusLine::kFit, 20);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_FilePath, wxALIGN_LEFT, StatusLine::kFixedPercentage, 30);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_Space, wxALIGN_LEFT, StatusLine::kStretch, 1);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_KeyStroke, wxALIGN_LEFT, StatusLine::kFixedPixel, 100);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_Space, wxALIGN_LEFT, StatusLine::kStretch, 1);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_Encoding, wxALIGN_LEFT, StatusLine::kFit, 20);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_FileFormat, wxALIGN_CENTER_HORIZONTAL, StatusLine::kFixedPixel, 60);
  SetFieldInfo(status_fields_[i++], StatusLine::kField_FileType, wxALIGN_RIGHT, StatusLine::kFixedPixel, 60);
}

} // namespace jil
