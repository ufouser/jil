#ifndef JIL_EDITOR_STYLE_H_
#define JIL_EDITOR_STYLE_H_
#pragma once

#include <map>
#include "wx/colour.h"
#include "wx/font.h"
#include "wx/string.h"

namespace jil {\
namespace editor {\

class Style {
public:
  // Some extra style items.
  static const std::string kNormal; // Normal text
  static const std::string kVisual; // Selected text
  static const std::string kNumber; // Line number area
  static const std::string kCaret; // or cursor
  static const std::string kCaretLine; // Current line
  static const std::string kSpace; // Tab and space

public:
  class Item {
  public:
    enum FontEffect {
      BOLD = 1,
      ITALIC = 2,
      UNDERLINE = 4,
    };

    Item(const std::string& name, const wxColour& fg, const wxColour& bg, int font = 0)
        : name_(name), fg_(fg), bg_(bg), font_(font) {
    }

    const std::string& name() const { return name_; }
    const wxColour& fg() const { return fg_; }
    const wxColour& bg() const { return bg_; }
    int font() const { return font_; }

  private:
    std::string name_;
    wxColour fg_;
    wxColour bg_;
    int font_;
  };

public:
  Style();
  ~Style();

  Item* Get(const std::string& name) const;
  void Add(Item* style_item);

private:
  typedef std::map<std::string, Item*> ItemMap;
  ItemMap item_map_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_STYLE_H_
