#ifndef JIL_APP_H_
#define JIL_APP_H_
#pragma once

#include <vector>
#include <map>
#include "wx/app.h"
#include "wx/ipc.h"
#include "wx/arrstr.h"
#include "base/compiler_specific.h"
#include "editor/theme.h"
#include "editor/option.h"
#include "editor/status_line.h"
#include "app/config.h"
#include "app/session.h"

class wxSingleInstanceChecker;

#define kAppName wxT("jil")
#define kAppDisplayName wxT("Jil") // TODO

namespace jil {\

namespace editor {\
class Style;
class FileType;
class Binding;
}
class BookBinding;

class Session;

class App : public wxApp {
public:
  App();
  virtual ~App();

  virtual bool OnInit() OVERRIDE;
  virtual int OnExit() OVERRIDE;

  // Get file type object by file type name.
  // If the file type doesn't exist, it will be created.
  editor::FileType* GetFileType(const wxString& ft_name);

  // TODO
  Session& session() { return session_; }

  // TODO
  int cjk_filters() const { return cjk_filters_; }
  const wxString& file_encoding() const { return file_encoding_; }
  const wxFont& font() const { return font_; }

  const std::vector<editor::StatusLine::FieldInfo>& status_fields() const {
    return status_fields_;
  }

protected:
  virtual void OnFatalException() OVERRIDE;

#if wxUSE_CMDLINE_PARSER

  // Called from OnInit() to add all supported options to the given parser.
  virtual void OnInitCmdLine(wxCmdLineParser& parser) OVERRIDE;

  // Called after successfully parsing the command line, return true
  // to continue and false to exit.
  virtual bool OnCmdLineParsed(wxCmdLineParser& parser) OVERRIDE;

  // Called if "--help" option was specified, return true to continue
  // and false to exit.
  virtual bool OnCmdLineHelp(wxCmdLineParser& parser) OVERRIDE;

  // Called if incorrect command line options were given, return
  // false to abort and true to continue.
  virtual bool OnCmdLineError(wxCmdLineParser& parser) OVERRIDE;

#endif // wxUSE_CMDLINE_PARSER

private:
  const wxString& GetPluginsDir() const;
  const wxString& GetUserDataDir() const;

  void SetCjkFilters(const std::string& cjk);

  void LoadOptions();
  void LoadEditOptions(Config* config, editor::Options& options);

  wxString GetThemeFile();
  bool LoadThemeFile(const wxString& file);
  bool LoadTheme();

  void LoadStyle();

  bool LoadBinding();

  bool LoadStatusFields();
  void UseDefaultStatusFields();

private:
  wxSingleInstanceChecker* instance_checker_;
  wxServer* server_;

  // Files specified via command line argument.
  // TODO: Avoid this.
  wxArrayString cmdline_files_;

  Session session_;

  int cjk_filters_;
  wxString file_encoding_;
  wxFont font_;
  wxString theme_name_;

  editor::SharedTheme theme_;
  editor::Style* style_;

  // Global options.
  editor::Options options_;

  std::vector<editor::FileType*> file_types_;

  editor::Binding* text_binding_;
  BookBinding* book_binding_;

  // Status fields for status line.
  std::vector<editor::StatusLine::FieldInfo> status_fields_;

  // Keep the accelerator entries for looking up menu label later, e.g., in OnUpdateUI.
  std::vector<wxAcceleratorEntry> accel_entries_;
};

DECLARE_APP(App)

} // namespace jil

#endif // JIL_APP_H_
