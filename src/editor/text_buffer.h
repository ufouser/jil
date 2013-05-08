#ifndef JIL_EDITOR_TEXT_BUFFER_H_
#define JIL_EDITOR_TEXT_BUFFER_H_
#pragma once

#include <string>
#include <vector>
#include <list>
#include <deque>
#include "wx/string.h"
#include "wx/filename.h"
#include "base/compiler_specific.h"
#include "editor/buffer_listener.h"
#include "editor/defs.h"
#include "editor/text_line.h"
#include "editor/text_range.h"
#include "editor/text_point.h"
#include "editor/seeker.h"

// A note about wrap:
// Wrap is an operation of the view instead of the model, because wrap needs
// to know the text clinet width.

namespace jil {
namespace editor {

class FileType;
class TextExtent;
class EditAction;
class InsertCharAction;

// Text buffer is the model of an opened text file.
// A view implemented BufferListener interface listens to the change of it.
// A text buffer may have several views.
class TextBuffer {
  //----------------------------------------------------------------------------
private:
  TextBuffer(FileType* file_type);

public:
  // Create an empty text buffer.
  static TextBuffer* Create(FileType* file_type, const wxString& file_encoding);

  // Create a text buffer for the given file.
  static TextBuffer* Create(const wxFileName& file_name_object,
                            FileType* file_type,
                            int cjk_filters,
                            const wxString& file_encoding);

  // Create a text buffer with the given text.
  static TextBuffer* Create(const std::wstring& text,
                            FileType* file_type,
                            const wxString& file_encoding);

  ErrorCode SaveFile();

  ~TextBuffer();

  //----------------------------------------------------------------------------

  // Example: "E:\doc\example.txt"
  wxString file_path_name() const;
  void set_file_path_name(const wxString& file_path_name);

  // Example: "example.txt"
  wxString file_name() const;
  // Example: "~/proj/jil/src/editor/"
  wxString file_path() const;

  const wxFileName& file_name_object() const {
    return file_name_object_;
  }
  void set_file_name_object(const wxFileName& file_name_object) {
    file_name_object_ = file_name_object;
  }

  const wxString& file_encoding() const { return file_encoding_; }
  void set_file_encoding(const wxString& file_encoding) {
    file_encoding_ = file_encoding;
  }

  FileFormat file_format() const { return file_format_; }
  void set_file_format(FileFormat file_format) { file_format_ = file_format; }

  FileType* file_type() const { return file_type_; }

  //----------------------------------------------------------------------------
  // Buffer states.

  // New buffer. No file path name.
  bool new_created() const;

  // Buffer's edited, but not saved yet.
  bool modified() const;

  // Read-only. Could be edited, but saving the change is not allowed.
  bool read_only() const;

  // The file is deleted in disk (after it's opened in Jil).
  bool deleted() const;

  //----------------------------------------------------------------------------

  Coord LineCount() const;

  TextLine& GetLine(Coord ln);
  const TextLine& GetLine(Coord ln) const;

  wchar_t GetChar(const TextPoint& point) const;

  size_t GetLineId(Coord ln) const;

  Coord LineLength(Coord ln) const;

  //----------------------------------------------------------------------------
  // Buffer change operations:

  std::wstring GetText(const TextRange& range) const;
private:
  void SetText(const std::wstring& text);

public:
  TextPoint InsertChar(const TextPoint& point, wchar_t c);
  void DeleteChar(const TextPoint& point, wchar_t* c = NULL);

  TextPoint InsertString(const TextPoint& point, const std::wstring& str);
  void DeleteString(const TextPoint& point, size_t count, std::wstring* str = NULL);

  // The new line will be with the given line number.
  void InsertLine(Coord ln, const std::wstring& line_data = L"");

  void DeleteLine(Coord ln, std::wstring* line_data = NULL);

  TextPoint InsertText(const TextPoint& point, const std::wstring& text);
  void DeleteText(const TextRange& range, std::wstring* text = NULL);

  //----------------------------------------------------------------------------
  // Listener operations:

  void AttachListener(BufferListener* listener);
  void DetachListener(BufferListener* listener);
  size_t ListenerCount() const { return listeners_.size(); }
  void Notify(ChangeType type, const ChangeData& data);

  void FreezeNotify();
  void ThawNotify();

  void NotifyEncodingChange();
  void NotifyFileNameChange();
  void NotifySaved();

  //----------------------------------------------------------------------------
  // Undo/redo

  // Return the added action or the previous action to which this action is merged.
  EditAction* AddEditAction(EditAction* edit_action);
  // Special handling for insert char action.
  // Insert_char actions added with this function might be merged later.
  void AddInsertCharAction(InsertCharAction* insert_char_action);

  //----------------------------------------------------------------------------

  // Bracket pairs: (), {}, [], <>, "", ''.
  bool GetBracketPairOuterRange(const TextRange& range,
                                TextRange& bracket_pair_range) const;
  bool GetBracketPairInnerRange(const TextRange& range,
                                TextRange& bracket_pair_range) const;

  bool IsBracketPairInnerRange(TextRange& bracket_pair_range) const;

  bool IncreaseRange(TextRange& range) const;

private:
  bool FindOpenBracket(const TextPoint& point, TextPoint& found_point) const;
  bool FindCloseBracket(const TextPoint& point, TextPoint& found_point) const;

private:
  // Merge continuously InsertChar actions to a InsertString action to
  // make the undo more practical.
  void MergeInsertCharActions();

public:
  EditAction* Undo();
  EditAction* Redo();

  // Do the last edit action again at the given point.
  EditAction* Repeat(const TextPoint& point);

  bool CanUndo() const;
  bool CanRedo() const;
  bool CanRepeat() const;

private:
  void ClearEditActionList(std::list<EditAction*>& edit_actions);

  EditAction* GetUndoEditAction();
  EditAction* PopUndoEditAction();
  void PushUndoEditAction(EditAction* edit_action);
  void PushRedoEditAction(EditAction* edit_action);

  void ClearEditActions();

  // Move current undo actions to "saved undo action list".
  void SaveUndoActions();

protected:
  void InsertLine(Coord ln, TextLine* line);
  void AppendLine(const std::wstring& line_data = L"");

private:
  size_t NewLineId();

public:
#ifdef __WXDEBUG__
  void LogLineLengthTable();
#endif // __WXDEBUG__
  Coord GetMaxLineLength() const;
private:
  void AddLineLength(size_t line_length);
  void RemoveLineLength(size_t line_length);
  void ShrinkLineLengthTable();

private:
  wxFileName file_name_object_;

  FileType* file_type_;

  // Encoding (charset) of the original file.
  wxString file_encoding_;

  FileFormat file_format_;

  // Buffer states:
  bool read_only_;
  bool deleted_;

  // Text buffer consists of text lines.
  typedef std::deque<TextLine*> TextLines;
  TextLines lines_;

  size_t line_id_;

  // A text buffer has one or more listeners.
  // Different listeners may want to listen to different changes of the buffer.
  // But in current implementation, a listener always listens to all changes.
  typedef std::vector<BufferListener*> Listeners;
  Listeners listeners_;
  bool notify_frozen_;

  // Undo/redo.

  std::list<EditAction*> undo_edit_actions_;
  std::list<EditAction*> redo_edit_actions_;

  // When buffer is saved, the undo actions are moved to this list.
  std::list<EditAction*> saved_undo_edit_actions_;

  size_t last_saved_undo_count_;

  // Continuously inserted chars should be undone by word.
  std::vector<InsertCharAction*> temp_insert_char_actions_;

  // Index is the line length, value is the count of lines with this length.
  // Example:
  // Suppose the buffer has the following five lines:
  // 1 xxx
  // 2 xx
  // 3
  // 4 xxxx
  // 5 xx
  // And the line length table will be:
  // Index  Value
  //   0      1     (line 3)
  //   1      0     (no line with size 1)
  //   2      2     (line 2 & 5)
  //   3      1     (line 1)
  //   4      1     (line 4)
  std::vector<long> line_length_table_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_TEXT_BUFFER_H_
