#include "editor/style.h"
#include "base/foreach.h"
#include "editor/lex.h"
#include "editor/color.h"

namespace jil {\
namespace editor {\

// static
const std::string Style::kNormal = "normal";
const std::string Style::kVisual = "visual";
const std::string Style::kNumber = "number";
const std::string Style::kCaretLine = "caret_line";
const std::string Style::kCaret = "caret";
const std::string Style::kSpace = "space";

Style::Style() {
}

Style::~Style() {
  ItemMap::iterator it = item_map_.begin(), end(item_map_.end());
  for (; it != end; ++it) {
    delete it->second;
  }
  item_map_.clear();
}

Style::Item* Style::Get(const std::string& name) const {
  ItemMap::const_iterator it = item_map_.find(name);
  if (it == item_map_.end()) {
    return NULL;
  }
  return it->second;
}

void Style::Add(Style::Item* style_item) {
  ItemMap::iterator it = item_map_.find(style_item->name());
  if (it == item_map_.end()) {
    item_map_[style_item->name()] = style_item;
  } else {
    delete it->second;
    it->second = style_item;
  }
}

} } // namespace jil::editor
