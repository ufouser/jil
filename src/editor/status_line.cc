#include "editor/status_line.h"
#include "wx/dcbuffer.h"
#include "wx/log.h"
#include "editor/text_window.h"
#include "editor/text_extent.h"
#include "editor/file_type.h"
#include "editor/text_buffer.h"
#include "editor/color.h"

namespace jil {
namespace editor {

BEGIN_EVENT_TABLE(StatusLine, wxPanel)
EVT_PAINT   (StatusLine::OnPaint)
EVT_SIZE    (StatusLine::OnSize)
END_EVENT_TABLE()

const int kPaddingX = 3;

StatusLine::StatusLine()
    : buffer_window_(NULL) {
}

bool StatusLine::Create(TextWindow* parent, wxWindowID id) {
  assert(theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  if (theme_->GetFont(FONT).IsOk()) {
    SetFont(theme_->GetFont(FONT));
  }

  buffer_window_ = parent;
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  return true;
}

StatusLine::~StatusLine() {
}

void StatusLine::AddField(FieldId field_id, wxAlignment align, SizeType size_type, int size_value) {
  FieldInfo field_info = { field_id, align, size_type, size_value, 0 };
  field_infos_.push_back(field_info);
}

void StatusLine::SetFields(const std::vector<FieldInfo>& field_infos) {
  field_infos_ = field_infos;
}

void StatusLine::NotifyChange(FieldId field_id) {
  wxRect field_rect = GetFieldRect(field_id);
  if (!field_rect.IsEmpty()) {
    RefreshRect(field_rect);
  }
}

// TODO: Move
static const wxString& FileFormatName(FileFormat ff) {
  static const wxString FileFormatNames[4] = {
    wxEmptyString,
    wxT("win"),
    wxT("unix"),
    wxT("mac"),
  };
  return FileFormatNames[ff];
}

void StatusLine::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  TextBuffer* buffer = buffer_window_->buffer();

  const wxRect rect = GetClientRect();

  dc.SetTextForeground(theme_->GetColor(FG));
  dc.SetFont(GetFont());

  wxRect update_rect = GetUpdateClientRect();
  update_rect.SetTop(rect.GetTop());
  update_rect.SetBottom(rect.GetBottom());

  // Background.

  wxRect bg_rect = update_rect;
  bg_rect.Inflate(1, 0);
  bg_rect.SetTop(bg_rect.GetTop() + 1); // 1px for the border.

  int color_delta = (bg_rect.GetHeight() + 2) / 3;
  wxColour bg_top = IncColor(theme_->GetColor(BG), color_delta);
  dc.GradientFillLinear(bg_rect, theme_->GetColor(BG), bg_top, wxNORTH);

  dc.SetPen(wxPen(theme_->GetColor(BORDER)));
  dc.DrawLine(update_rect.GetLeft(), update_rect.GetTop(), update_rect.GetRight() + 1, update_rect.GetTop());

  // Foreground.

  dc.SetBrush(*wxTRANSPARENT_BRUSH);

  int char_height = 0;
  dc.GetTextExtent(wxT("T"), NULL, &char_height, NULL, NULL);
  int label_y = rect.GetTop() + (rect.GetHeight() - char_height) / 2;

  int x = rect.GetLeft();
  for (size_t i = 0; i < field_infos_.size(); ++i) {
    FieldInfo& field_info = field_infos_[i];
    wxRect field_rect(x, rect.GetTop(), field_info.size, rect.GetHeight());

    if (field_rect.GetLeft() >= update_rect.GetRight()) { // At the right of update rect.
      break;
    }

    if (field_rect.GetRight() <= update_rect.GetLeft()) { // At the left of update rect.
      x += field_info.size;
      continue;
    }

#ifdef __WXDEBUG__
    //dc.DrawRectangle(x + 1, rect.GetTop() + 2, field_info.size - 2, rect.GetHeight() - 4);
#endif

    wxString label = GetFieldLabel(field_info.field_id);
    field_rect.Deflate(kPaddingX, 0);

    int expected_size = 0;
    dc.GetTextExtent(label, &expected_size, NULL, NULL, NULL);

    if (expected_size > field_rect.GetWidth()) {
      label = label.Mid(0, TailorLabel(dc, label, field_rect.GetWidth()));
    }

    dc.DrawLabel(label, field_rect, field_info.align | wxALIGN_CENTER_VERTICAL);

    x += field_info.size;
  }
}

void StatusLine::OnSize(wxSizeEvent& evt) {
  evt.Skip();

  if (GetClientSize().x < kUnreadyWindowSize) {
    return;
  }

  UpdateFieldSizes();
  Refresh();
}

void StatusLine::UpdateFieldSizes() {
  const int client_size = GetClientSize().GetX();
  int size_left = client_size;
  int stretch_field_count = 0;

  for (size_t i = 0; i < field_infos_.size(); ++i) {
    FieldInfo& field_info = field_infos_[i];

    switch (field_info.size_type) {
      case kFit:
        GetTextExtent(GetFieldLabel(field_info.field_id), &field_info.size, NULL, NULL, NULL);
        field_info.size += field_info.size_value;
        field_info.size += kPaddingX + kPaddingX;
        break;

      case kFixedPixel:
        field_info.size = field_info.size_value;
        break;

      case kFixedPercentage:
        field_info.size = field_info.size_value * client_size / 100; // TODO: round()
        break;

      case kStretch:
        field_info.size = 0; // Reset size
        ++stretch_field_count;
        break;
    }

    if (field_info.size > 0) {
      size_left -= field_info.size;
    }
  }

  if (stretch_field_count > 0 && size_left > 0) { // size_left > stretch_field_count ?
    int stretch_size = size_left / stretch_field_count;

    for (size_t i = 0; i < field_infos_.size(); ++i) {
      FieldInfo& field_info = field_infos_[i];

      if (field_info.size_type == kStretch) {
        field_info.size = stretch_size;
      }
    }
  }
}

wxString StatusLine::GetFieldLabel(FieldId& field_id) {
  TextBuffer* buffer = buffer_window_->buffer();

  wxString label;

  switch (field_id) {
    case kField_Encoding:
      label = buffer->file_encoding();
      break;

    case kField_FileFormat:
      label = FileFormatName(buffer->file_format());
      break;

    case kField_FileName:
      label = buffer->file_name();
      break;

    case kField_FilePath:
      label = buffer->file_path();
      break;

    case kField_FileType:
      label = buffer->file_type()->name();
      break;

    case kField_Caret:
      {
        TextPoint caret_point = buffer_window_->caret_point();
        label = wxString::Format(wxT("line %d/%d, column %d"), caret_point.y, buffer->LineCount(), caret_point.x);
      }
      break;

    case kField_KeyStroke:
      {
        Key leader_key = buffer_window_->leader_key();
        if (!leader_key.IsEmpty()) {
          label = leader_key.ToString();
        }
      }
      break;

    case kField_Space:
      // Do nothing.
      break;

    default:
      break;
  }

  return label;
}

wxRect StatusLine::GetFieldRect(FieldId& field_id) const {
  const wxRect client_rect = GetClientRect();
  int x = client_rect.GetLeft();

  for (size_t i = 0; i < field_infos_.size(); ++i) {
    const FieldInfo& field_info = field_infos_[i];
    if (field_info.field_id == field_id) {
      return wxRect(x, client_rect.GetTop(), field_info.size, client_rect.GetHeight());
    }
    x += field_info.size;
  }

  return wxRect();
}

} } // namespace jil::editor
