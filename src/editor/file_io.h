#ifndef JIL_EDITOR_FILE_IO_H_
#define JIL_EDITOR_FILE_IO_H_
#pragma once

class wxString;
#include <string>

namespace jil {\
namespace editor {\

// Read file in bytes.
// Return 0 on success, 1 on IO error and 2 on exceeding bytes limit.
int ReadBytes(const wxString& filename, std::string& bytes, size_t max_bytes = std::wstring::npos);

// Save file in bytes.
// Return 0 on success, 1 on IO error.
int SaveBytes(const wxString& filename, const char* bom, const std::string& bytes);

} } // namespace jil::editor

#endif // JIL_EDITOR_FILE_IO_H_
