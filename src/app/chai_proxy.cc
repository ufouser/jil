#include "app/chai_proxy.h"
// Note:
// If include chaiscript before wx headers, there will be link errors like:
// Unresolved external ... GetClassInfoW ... in VC++.
#include "chaiscript/chaiscript.hpp"
#include "wx/defs.h"
#include "wx/stdpaths.h"
#include "wx/colour.h"
#include "wx/msgdlg.h"
#include "wx/log.h"
#include "editor/binding.h"
#include "editor/style.h"
#include "editor/file_io.h"
#include "editor/text_window.h"
#include "app/font_util.h"
#include "app/binding_config.h"

namespace jil {\

using namespace editor;

////////////////////////////////////////////////////////////////////////////////
// Wrappers.

static void Bind(Binding* binding, int modes, const std::string& cmd, const std::string& keys) {
  wxLogDebug(wxT("Bind: %s -> %s"), cmd.c_str(), keys.c_str());
  ParseBinding(cmd, keys, modes, binding);
}

static wxColour MakeColor(int r, int g, int b) {
  return wxColour(r, g, b);
}

static wxColour MakeColor(const std::string& color_name) {
  return wxColour(wxString::FromAscii(color_name.c_str()));
}

static wxFont MakeFont(int point_size, const std::string& facename, bool bold, bool italic) {
  return GetGlobalFont(point_size, facename, bold, italic);
}

////////////////////////////////////////////////////////////////////////////////

ChaiProxy::ChaiProxy()
    : chai_(new chaiscript::ChaiScript)
    , binding_(NULL) {
}

ChaiProxy::~ChaiProxy() {
  delete chai_;
}

void ChaiProxy::Init() {
  using namespace chaiscript;

  assert(binding_ != NULL);

  //----------------------------------------------------------------------------
  // General

  // wxString -> String
  // Note: ChaiScript has no "C string", it doesn't make sense to export
  // constructor "wxString (const char*)".
  chai_->add(user_type<wxString>(), "String");
  chai_->add(constructor<wxString ()>(), "String");
  chai_->add(constructor<wxString (const std::string&)>(), "String");
  chai_->add(constructor<wxString (const wxString&)>(), "String");

  // wxColour -> Color
  chai_->add(user_type<wxColour>(), "Color");
  chai_->add(constructor<wxColour ()>(), "Color");
  chai_->add(constructor<wxColour (const wxColour&)>(), "Color");
  chai_->add(fun<wxColour (int, int, int)>(&MakeColor), "color");
  chai_->add(fun<wxColour (const std::string&)>(&MakeColor), "color");
  chai_->add(fun(&wxColour::operator=), "=");

  // wxFont -> Font
  chai_->add(user_type<wxFont>(), "Font");
  chai_->add(constructor<wxFont ()>(), "Font");
  chai_->add(constructor<wxFont (const wxFont&)>(), "Font");
  chai_->add(fun<wxFont (int, const std::string&, bool, bool)>(&MakeFont), "font");
  chai_->add(fun(&wxFont::operator=), "=");

  //----------------------------------------------------------------------------
  // Bind

  chai_->add(fun(&Bind, binding_, kAllModes), "bind");
  chai_->add(fun(&Bind, binding_, kNormalMode), "nbind");
  chai_->add(fun(&Bind, binding_, kVisualMode), "vbind");
}

bool ChaiProxy::EvalFile(const wxString& script_file) {
  std::string script_text;
  int result = ReadBytes(script_file, script_text);
  if (result != 0) {
    return false;
  }

  try {
    chai_->eval(script_text);
  } catch (std::exception& e) {
    // TODO: Error handling.
    //const char* what = e.what();
    return false;
  }

  return true;
}

} // namespace jil
