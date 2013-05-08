#ifndef JIL_CONFIG_H_
#define JIL_CONFIG_H_
#pragma once

// C++ wrapper for libconfig C interface.

#include <string>
#include "wx/string.h"
#include "wx/colour.h"
#include "wx/gdicmn.h"
#include "wx/font.h"
#include "libconfig/libconfig.h"

namespace jil {\

class Setting {
public:
  enum Type {
    kNone = CONFIG_TYPE_NONE,
    kGroup = CONFIG_TYPE_GROUP,
    kInt = CONFIG_TYPE_INT,
    kInt64 = CONFIG_TYPE_INT64,
    kFloat = CONFIG_TYPE_FLOAT,
    kString = CONFIG_TYPE_STRING,
    kBool = CONFIG_TYPE_BOOL,
    kArray = CONFIG_TYPE_ARRAY,
    kList = CONFIG_TYPE_LIST,
  };

  Setting(config_setting_t* ref = NULL)
      : ref_(ref) {
  }

  void set_ref(config_setting_t* ref = NULL) {
    ref_ = ref;
  }

  operator bool() const { return ref_ != NULL; }

  const char* name() const {
    return ref_->name;
  }

  int type() const {
    return ref_->type;
  }

  unsigned int line() const {
    return ref_->line;
  }

  const char* file() const {
    return ref_->file;
  }

  // For aggregate setting.

  int size() const {
    return config_setting_length(ref_);
  }

  Setting Get(int index) const {
    return Setting(config_setting_get_elem(ref_, index));
  }

  Setting Get(const char* name) const {
    return Setting(config_setting_get_member(ref_, name));
  }

  Setting Get(const char* name, int type) {
    return GetOrRecreate(name, type);
  }

  Setting Add(const char* name, int type) {
    return Setting(config_setting_add(ref_, name, type));
  }

  void Remove(const char* name) {
    config_setting_remove(ref_, name);
  }

  // For scalar setting.

  int         GetInt() const;
  void        SetInt(int value);

  bool        GetBool() const;
  void        SetBool(bool value);

  const char* GetString() const;
  void        SetString(const char* value);

  // For group setting.

  int         GetInt(const char* name, int default_value = 0) const;
  void        SetInt(const char* name, int value);

  bool        GetBool(const char* name, bool default_value = false) const;
  void        SetBool(const char* name, bool value);

  const char* GetString(const char* name) const;
  void        SetString(const char* name, const char* value);

  wxColour    GetColor(const char* name) const;
  wxColour    GetColor(const char* name, const wxColour& default_value) const;
  void        SetColor(const char* name, const wxColour& value);

  wxFont      GetFont(const char* name) const;
  wxFont      GetFont(const char* name, int default_size, const wxString& default_face) const;
  void        SetFont(const char* name, const wxFont& font);

  bool        GetRect(const char* name, wxRect& rect) const;
  void        SetRect(const char* name, const wxRect& rect);

private:
  Setting GetOrRecreate(const char* name, int type);

private:
  // A reference only.
  config_setting_t* ref_;
};

class Config {
public:
  Config() {
    config_init(&cfg_);
  }

  ~Config() {
    config_destroy(&cfg_);
  }

  bool Load(const wxString& filename);
  bool Save(const wxString& filename);

  Setting root() const {
    return Setting(cfg_.root);
  }

  Setting Lookup(const char* path) const {
    return Setting(config_lookup(&cfg_, path));
  }

private:
  config_t cfg_;
};

} // namespace jil

#endif // JIL_CONFIG_H_
