#include "app/config.h"
#include "wx/log.h"
#include "app/font_util.h"

namespace jil {\

// Setting

int Setting::GetInt() const {
  return config_setting_get_int(ref_);
}

void Setting::SetInt(int value) {
  config_setting_set_int(ref_, value);
}

bool Setting::GetBool() const {
  return config_setting_get_bool(ref_) != 0;
}

void Setting::SetBool(bool value) {
  config_setting_set_bool(ref_, value);
}

const char* Setting::GetString() const {
  return config_setting_get_string(ref_);
}

void Setting::SetString(const char* value) {
  config_setting_set_string(ref_, value);
}

int Setting::GetInt(const char* name, int default_value) const {
  int value = 0;
  if (config_setting_lookup_int(ref_, name, &value) == CONFIG_TRUE) {
    return value;
  }
  return default_value;
}

void Setting::SetInt(const char* name, int value) {
  Setting child_setting = GetOrRecreate(name, CONFIG_TYPE_INT);
  child_setting.SetInt(value);
}

bool Setting::GetBool(const char* name, bool default_value) const {
  int value = 0;
  if (config_setting_lookup_bool(ref_, name, &value) == CONFIG_TRUE) {
    return value != 0;
  }
  return default_value;
}

void Setting::SetBool(const char* name, bool value) {
  Setting child_setting = GetOrRecreate(name, CONFIG_TYPE_BOOL);
  child_setting.SetBool(value);
}

const char* Setting::GetString(const char* name) const {
  const char* value = NULL;
  if (config_setting_lookup_string(ref_, name, &value) == CONFIG_TRUE) {
    return value;
  }
  return "";
}

void Setting::SetString(const char* name, const char* value) {
  Setting child_setting = GetOrRecreate(name, CONFIG_TYPE_STRING);
  child_setting.SetString(value);
}

wxColour Setting::GetColor(const char* name) const {
  wxColour color;
  wxString color_str = wxString::FromAscii(GetString(name));
  color.Set(color_str);
  return color;
}

wxColour Setting::GetColor(const char* name, const wxColour& default_value) const {
  wxColour color = GetColor(name);
  if (!color.IsOk()) {
    color = default_value;
  }
  return color;
}

// TODO
void Setting::SetColor(const char* name, const wxColour& value) {
  if (!value.IsOk()) {
    return;
  }
  SetString(name, value.GetAsString().ToAscii().data());
}

wxFont Setting::GetFont(const char* name) const {
  wxFont font;
  font.SetNativeFontInfoUserDesc(wxString::FromUTF8(GetString(name)));
  return font;
}

wxFont Setting::GetFont(const char* name, int default_size, const wxString& default_face) const {
  wxFont font = GetFont(name);
  if (!font.IsOk()) {
    font = GetGlobalFont(default_size, default_face);
  }
  return font;
}

void Setting::SetFont(const char* name, const wxFont& font) {
  SetString(name, font.GetNativeFontInfoUserDesc().ToUTF8().data());
}

bool Setting::GetRect(const char* name, wxRect& rect) const {
  Setting rect_setting = Get(name);
  if (!rect_setting) {
    return false;
  }

  if (rect_setting.type() != Setting::kArray || rect_setting.size() != 4) {
    return false;
  }

  rect.x = rect_setting.Get(0).GetInt();
  rect.y = rect_setting.Get(1).GetInt();
  rect.width = rect_setting.Get(2).GetInt();
  rect.height = rect_setting.Get(3).GetInt();

  return true;
}

void Setting::SetRect(const char* name, const wxRect& rect) {
  Remove(name);
  Setting rect_setting = Add(name, kArray);
  rect_setting.Add(NULL, kInt).SetInt(rect.x);
  rect_setting.Add(NULL, kInt).SetInt(rect.y);
  rect_setting.Add(NULL, kInt).SetInt(rect.width);
  rect_setting.Add(NULL, kInt).SetInt(rect.height);
}

Setting Setting::GetOrRecreate(const char* name, int type) {
  Setting child = Get(name);
  if (child && child.type() != type) {
    Remove(name);
    child.set_ref(NULL);
  }
  if (!child) {
    child = Add(name, type);
  }
  return child;
}

// Config

bool Config::Load(const wxString& filename) {
  FILE* config_file = wxFopen(filename, wxT("r"));
  if (config_file == NULL) {
    wxLogInfo(wxT("Failed to open config file to read: %s"), filename.c_str());
    return false;
  }

  int read_code = config_read(&cfg_, config_file);
  fclose(config_file);

  if (read_code == 0) {
    wxLogDebug(wxT("Failed to read config: L%d, %s."),
               config_error_line(&cfg_),
               wxString::FromUTF8(config_error_text(&cfg_)));
    return false;
  }

  return true;
}

bool Config::Save(const wxString& filename) {
  FILE* config_file = wxFopen(filename, wxT("w"));
  if (config_file == NULL) {
    wxLogInfo(wxT("Failed to open config file to write: %s"), filename.c_str());
    return false;
  }

  config_write(&cfg_, config_file);
  fclose(config_file);
  return true;
}

} // namespace jil
