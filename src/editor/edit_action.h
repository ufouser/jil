#ifndef JIL_EDITOR_EDIT_ACTION_H_
#define JIL_EDITOR_EDIT_ACTION_H_
#pragma once

// Support undo & redo based on Command pattern.

// Ideas:
// - The timestamp for each edit action can be used to replay.
// - Old edit actions can be serialized to disk to save memory.

#include <vector>
#include <list>
#include <map>
#include "wx/string.h"
#include "wx/datetime.h"
#include "base/compiler_specific.h"
#include "editor/defs.h"
#include "editor/text_range.h"
#include "editor/seeker.h"

namespace jil {
namespace editor {

class TextBuffer;

////////////////////////////////////////////////////////////////////////////////

class DeleteAction;
class DeleteRangeAction;

class EditAction {
public:
  EditAction(TextBuffer* buffer, const TextPoint& point);
  EditAction(const EditAction& rhs, const TextPoint& point);
  virtual ~EditAction();

  bool ever_saved() const { return ever_saved_; }
  void set_ever_saved(bool ever_saved) { ever_saved_ = ever_saved; }

  const wxDateTime& timestamp() const { return timestamp_; }

  const TextPoint& point() const { return point_; }
  const TextPoint& delta_point() const { return delta_point_; }

  TextPoint GetExecPoint() const {
    return point_ + delta_point_;
  }

  virtual void Exec() = 0;
  virtual void Undo() = 0;

  virtual bool CanRepeat() const { return false; }

  virtual EditAction* Clone(const TextPoint& point) { return NULL; }

  // TODO
  virtual DeleteAction* AsDeleteAction() { return NULL; }
  virtual DeleteRangeAction* AsDeleteRangeAction() { return NULL; }

protected:
  bool ever_saved_;

  // Text buffer on which this command is executed.
  TextBuffer* buffer_;

  // The time when this command is created.
  wxDateTime timestamp_;

  // Edit (insert, delete, etc.) point.
  TextPoint point_;

  // delta_point_ + point_ = the new caret point after this command is executed.
  TextPoint delta_point_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertCharAction : public EditAction {
public:
  InsertCharAction(TextBuffer* buffer, const TextPoint& point, wchar_t c);
  InsertCharAction(const InsertCharAction& rhs, const TextPoint& point);

  wchar_t c() const { return c_; }

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return true; }

  virtual EditAction* Clone(const TextPoint& point) OVERRIDE {
    return new InsertCharAction(*this, point);
  }

private:
  wchar_t c_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertStringAction : public EditAction {
public:
  InsertStringAction(TextBuffer* buffer, const TextPoint& point, const std::wstring& str);
  InsertStringAction(const InsertStringAction& rhs, const TextPoint& point);
  virtual ~InsertStringAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return true; }

  virtual EditAction* Clone(const TextPoint& point) OVERRIDE {
    return new InsertStringAction(*this, point);
  }

private:
  std::wstring str_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertTextAction : public EditAction {
public:
  InsertTextAction(TextBuffer* buffer, const TextPoint& point, const std::wstring& text);
  InsertTextAction(const InsertTextAction& rhs, const TextPoint& point);
  virtual ~InsertTextAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return true; }

  virtual EditAction* Clone(const TextPoint& point) OVERRIDE {
    return new InsertTextAction(*this, point);
  }

private:
  std::wstring text_;
};

////////////////////////////////////////////////////////////////////////////////

// Delete a single line.
// TODO: DeleteAction(kLine, kWhole)?
class DeleteLineAction : public EditAction {
public:
  DeleteLineAction(TextBuffer* buffer, const TextPoint& point);
  virtual ~DeleteLineAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return false; }

private:
  // The line deleted.
  std::wstring line_data_;
  bool is_last_line_;
};

////////////////////////////////////////////////////////////////////////////////

class DeleteAction : public EditAction {
public:
  DeleteAction(TextBuffer* buffer, const TextPoint& point, TextUnit text_unit, SeekType seek_type);
  virtual ~DeleteAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return true; }

  virtual EditAction* Clone(const TextPoint& point) OVERRIDE {
    return new DeleteAction(buffer_, point, text_unit_, seek_type_);
  }

  virtual DeleteAction* AsDeleteAction() { return this; }

  TextUnit text_unit() const { return text_unit_; }
  SeekType seek_type() const { return seek_type_; }

  size_t count() const { return count_; }
  void set_count(size_t count) { count_ = count; }

  bool Merge(DeleteAction* rhs);
  void DeleteText(const TextRange& range);

private:
  std::wstring text_;
  TextUnit text_unit_;
  SeekType seek_type_;
  size_t count_;
};

////////////////////////////////////////////////////////////////////////////////

class DeleteRangeAction : public EditAction {
public:
  DeleteRangeAction(TextBuffer* buffer, const TextRange& range, TextDir dir, bool selected);
  virtual ~DeleteRangeAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;

  virtual bool CanRepeat() const OVERRIDE { return false; }

  virtual DeleteRangeAction* AsDeleteRangeAction() OVERRIDE { return this; }

  const TextRange& range() const { return range_; }
  TextDir dir() const { return dir_; }
  bool selected() const { return selected_; }

private:
  TextRange range_;
  TextDir dir_;
  std::wstring text_;
  bool selected_;
};

////////////////////////////////////////////////////////////////////////////////

class NewLineBelowAction : public EditAction {
public:
  NewLineBelowAction(TextBuffer* buffer, const TextPoint& point);
  virtual ~NewLineBelowAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;
};

////////////////////////////////////////////////////////////////////////////////

class NewLineAboveAction : public EditAction {
public:
  NewLineAboveAction(TextBuffer* buffer, const TextPoint& point);
  virtual ~NewLineAboveAction();

  virtual void Exec() OVERRIDE;
  virtual void Undo() OVERRIDE;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_EDIT_ACTION_H_
