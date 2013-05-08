#ifndef JIL_BINDING_CONFIG_H_
#define JIL_BINDING_CONFIG_H_
#pragma once

// Load binding config file.

class wxString;

namespace jil {\

namespace editor {\
class Binding;
}
class BookBinding;

class BindingConfig {
public:
  BindingConfig(editor::Binding* text_binding, BookBinding* book_binding)
      : text_binding_(text_binding)
      , book_binding_(book_binding) {
  }

  bool Load(const wxString& file);

private:
  editor::Binding* text_binding_;
  BookBinding* book_binding_;
};

} // namespace jil

#endif // JIL_BINDING_CONFIG_H_
