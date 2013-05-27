#include "editor/edit_action.h"
#include <utility>
#include "base/foreach.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

EditAction::EditAction(TextBuffer* buffer, const TextPoint& point)
    : ever_saved_(false)
    , buffer_(buffer)
    , timestamp_(wxDateTime::UNow())
    , point_(point)
    , delta_point_(0, 0) {
}

EditAction::EditAction(const EditAction& rhs, const TextPoint& point)
    : ever_saved_(false)
    , buffer_(rhs.buffer_)
    , timestamp_(wxDateTime::UNow())
    , point_(point)
    , delta_point_(0, 0) {
}

EditAction::~EditAction() {
}

////////////////////////////////////////////////////////////////////////////////

InsertCharAction::InsertCharAction(TextBuffer* buffer, const TextPoint& point, wchar_t c)
    : EditAction(buffer, point)
    , c_(c) {
}

InsertCharAction::InsertCharAction(const InsertCharAction& rhs, const TextPoint& point)
    : EditAction(rhs, point)
    , c_(rhs.c_) {
}

void InsertCharAction::Exec() {
  if (c_ == LF) {
    delta_point_.x = -point_.x;
    delta_point_.y = 1;
  } else {
    delta_point_.x = 1;
  }

  buffer_->InsertChar(point_, c_);
}

void InsertCharAction::Undo() {
  buffer_->DeleteChar(point_);
}

////////////////////////////////////////////////////////////////////////////////

InsertStringAction::InsertStringAction(TextBuffer* buffer, const TextPoint& point, const std::wstring& str)
    : EditAction(buffer, point)
    , str_(str) {
}

InsertStringAction::InsertStringAction(const InsertStringAction& rhs, const TextPoint& point)
    : EditAction(rhs, point)
    , str_(rhs.str_) {
}

InsertStringAction::~InsertStringAction() {
}

void InsertStringAction::Exec() {
  buffer_->InsertString(point_, str_);
  delta_point_.x = static_cast<int>(str_.size());
}

void InsertStringAction::Undo() {
  buffer_->DeleteString(point_, str_.size());
}

////////////////////////////////////////////////////////////////////////////////

InsertTextAction::InsertTextAction(TextBuffer* buffer, const TextPoint& point, const std::wstring& text)
    : EditAction(buffer, point)
    , text_(text) {
}

InsertTextAction::InsertTextAction(const InsertTextAction& rhs, const TextPoint& point)
    : EditAction(rhs, point)
    , text_(rhs.text_) {
}

InsertTextAction::~InsertTextAction() {
}

void InsertTextAction::Exec() {
  TextPoint last_point = buffer_->InsertText(point_, text_);
  delta_point_ = last_point - point_;
}

void InsertTextAction::Undo() {
  buffer_->DeleteText(TextRange(point_, point_ + delta_point_), NULL);
}

////////////////////////////////////////////////////////////////////////////////

DeleteLineAction::DeleteLineAction(TextBuffer* buffer, const TextPoint& point)
    : EditAction(buffer, point)
    , is_last_line_(false) {
}

DeleteLineAction::~DeleteLineAction() {
}

void DeleteLineAction::Exec() {
  assert(point_.y <= buffer_->LineCount());

  is_last_line_ = buffer_->LineCount() == 1;

  buffer_->DeleteLine(point_.y, &line_data_);

  int y = point_.y;
  if (y > buffer_->LineCount()) {
    y = buffer_->LineCount();
    delta_point_.y = -1;
  }
  int line_after_length = buffer_->LineLength(y);
  if (point_.x <= line_after_length) {
    delta_point_.x = 0; // Keep the point x.
  } else {
    // The line after is too short.
    delta_point_.x = line_after_length - point_.x; // < 0
  }
}

void DeleteLineAction::Undo() {
  if (is_last_line_) {
    buffer_->InsertString(TextPoint(0, point_.y), line_data_);
  } else {
    buffer_->InsertLine(point_.y, line_data_);
  }
}

////////////////////////////////////////////////////////////////////////////////

DeleteAction::DeleteAction(TextBuffer* buffer, const TextPoint& point, TextUnit text_unit, SeekType seek_type)
    : EditAction(buffer, point)
    , seek_type_(seek_type)
    , text_unit_(text_unit)
    , count_(1) {
}

DeleteAction::~DeleteAction() {
}

void DeleteAction::Exec() {
  TextPoint another_point = Seek(buffer_, point_, text_unit_, seek_type_, count_);

  if (another_point < point_) {
    DeleteText(TextRange(another_point, point_));
    delta_point_ = another_point - point_;
  } else if (another_point > point_) {
    DeleteText(TextRange(point_, another_point));
  }
}

void DeleteAction::DeleteText(const TextRange& range) {
  if (!text_.empty()) { // Ever executed.
    buffer_->DeleteText(range);
  } else {
    buffer_->DeleteText(range, &text_);
  }
}

void DeleteAction::Undo() {
  if (!text_.empty()) {
    buffer_->InsertText(point_ + delta_point_, text_);
  }
}

bool DeleteAction::Merge(DeleteAction* next_delete_action) {
  assert(text_unit_ == next_delete_action->text_unit_ &&
         seek_type_ == next_delete_action->seek_type_ &&
         ever_saved_ == next_delete_action->ever_saved_);

  bool merged = false;

  if (text_unit_ == kChar || text_unit_ == kWord) { // TODO: kLine
    if (seek_type_ == kPrev) {
      text_ = next_delete_action->text_ + text_;
      merged = true;
    } else if (seek_type_ == kNext) {
      text_ += next_delete_action->text_;
      merged = true;
    }
  }

  if (merged) {
    count_ += next_delete_action->count_;
    delta_point_ += next_delete_action->delta_point_;
    timestamp_ = next_delete_action->timestamp_;
  }

  return merged;
}

////////////////////////////////////////////////////////////////////////////////

DeleteRangeAction::DeleteRangeAction(TextBuffer* buffer, const TextRange& range, TextDir dir, bool selected)
    : EditAction(buffer, dir == kForward ? range.last_point() : range.first_point())
    , range_(range)
    , dir_(dir)
    , selected_(selected) {
}

DeleteRangeAction::~DeleteRangeAction() {
}

void DeleteRangeAction::Exec() {
  if (range_.Empty()) {
    return;
  }

  if (!text_.empty()) { // Ever executed.
    buffer_->DeleteText(range_);
  } else {
    buffer_->DeleteText(range_, &text_);
  }

  if (dir_ == kForward) {
    delta_point_ = range_.first_point() - range_.last_point();
  }
}

void DeleteRangeAction::Undo() {
  if (!text_.empty()) {
    buffer_->InsertText(range_.first_point(), text_);
  }
}

////////////////////////////////////////////////////////////////////////////////

NewLineBelowAction::NewLineBelowAction(TextBuffer* buffer, const TextPoint& point)
    : EditAction(buffer, point) {
}

NewLineBelowAction::~NewLineBelowAction() {
}

void NewLineBelowAction::Exec() {
  buffer_->InsertLine(point_.y + 1);
  delta_point_.x = -point_.x;
  delta_point_.y = 1;
}

void NewLineBelowAction::Undo() {
  buffer_->DeleteLine(point_.y + 1);
}

////////////////////////////////////////////////////////////////////////////////

NewLineAboveAction::NewLineAboveAction(TextBuffer* buffer, const TextPoint& point)
    : EditAction(buffer, point) {
}

NewLineAboveAction::~NewLineAboveAction() {
}

void NewLineAboveAction::Exec() {
  buffer_->InsertLine(point_.y);
  delta_point_.x = -point_.x;
}

void NewLineAboveAction::Undo() {
  buffer_->DeleteLine(point_.y);
}

} } // namespace jil::editor
