#ifndef JIL_CHAI_PROXY_H_
#define JIL_CHAI_PROXY_H_
#pragma once

class wxString;

namespace chaiscript {\
class ChaiScript;
}

namespace jil {\

namespace editor {\
class Binding;
}

class ChaiProxy {
public:
  ChaiProxy();
  ~ChaiProxy();

  void set_binding(editor::Binding* binding) {
    binding_ = binding;
  }

  // Initialize ChaiScript.
  // Export variables, functions and classes to ChaiScript.
  void Init();

  bool EvalFile(const wxString& script_file);

private:
  chaiscript::ChaiScript* chai_;
  editor::Binding* binding_;
};

} // namespace jil

#endif // JIL_CHAI_PROXY_H_
