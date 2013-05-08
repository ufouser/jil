#ifndef JIL_EDITOR_STATUS_LINE_H_
#define JIL_EDITOR_STATUS_LINE_H_
#pragma once

#include "wx/panel.h"
#include <vector>
#include "editor/theme.h"

namespace jil {\
namespace editor {\

class TextWindow;

class StatusLine : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BORDER = 0,
    FG,
    BG,
    COLOR_COUNT
  };

  enum FontId {
    FONT = 0,
    FONT_COUNT
  };

  enum FieldId {
    kField_Cwd = 0,
    kField_Encoding,
    kField_FileFormat,
    kField_FileName,
    kField_FilePath,
    kField_FileType,
    kField_Caret,
    kField_KeyStroke,
    kField_Space,

    kField_Count,
  };

  enum SizeType {
    kFit,
    kFixedPixel,
    kFixedPercentage,
    kStretch
  };

  struct FieldInfo {
    FieldId field_id;
    wxAlignment align;
    SizeType size_type;
    // For different size types, different meanings of size value:
    // kFit -> padding
    // kFixedPixel -> pixels
    // kFixedPercentage -> % x 100
    // kStretch -> stretch factor
    int size_value;
    // Actually size.
    int size;
  };

public:
  StatusLine();
  bool Create(TextWindow* parent, wxWindowID id);
  virtual ~StatusLine();

  void set_theme(const SharedTheme& theme) {
    theme_ = theme;
  }

  void AddField(FieldId field_id, wxAlignment align, SizeType size_type, int size_value);

  void SetFields(const std::vector<FieldInfo>& field_infos);

  void NotifyChange(FieldId field_id);

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);

private:
  void UpdateFieldSizes();

  wxString GetFieldLabel(FieldId& field_id);

  // Get field rect according to its size and the client rect.
  // If the field is not found, the rect will be empty.
  wxRect GetFieldRect(FieldId& field_id) const;

private:
  SharedTheme theme_;

  TextWindow* buffer_window_;

  std::vector<FieldInfo> field_infos_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_STATUS_LINE_H_
