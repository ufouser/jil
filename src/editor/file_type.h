#ifndef JIL_EDITOR_FILE_TYPE_H_
#define JIL_EDITOR_FILE_TYPE_H_
#pragma once

#include "wx/string.h"
#include "editor/option.h"

namespace jil {
namespace editor {

class FileType {
public:
  FileType(const wxString& name);
  ~FileType();

  const wxString& name() const { return name_; }

  Options& options() { return options_; }

  // Load file type plugins.
  void Load();

private:
  // c, cpp, python, etc.
  wxString name_;

  // TODO
  // File type specific values:
  // - delimiters (for dividing word)
  // - combined operators ("==", "<=", etc.)
  // - ...

  Options options_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_FILE_TYPE_H_
