#ifndef JIL_EDITOR_KEY_H_
#define JIL_EDITOR_KEY_H_
#pragma once

#include "wx/string.h"

namespace jil {\
namespace editor {\

// A key consists of a key code and optionally the modifiers. It could also
// has a leader key.
// Examples:
// - Ctrl+J
// - Ctrl+K, Ctrl+K
// - Ctrl+K, U
// - Ctrl+Shift+K
class Key {
public:
  Key() : data_(0) {
  }

  Key(int code, int modifiers = wxMOD_NONE) {
    Set(code, modifiers);
  }

  Key(int leader_code, int leader_modifiers, int code, int modifiers = wxMOD_NONE) {
    Set(leader_code, leader_modifiers, code, modifiers);
  }

  int data() const { return data_; }

  int code() const { return data_ & 0xff; }
  int modifiers() const { return (data_ & 0xff00) >> 8; }
  int leader_code() const { return (data_ & 0xff0000) >> 16; }
  int leader_modifiers() const { return (data_ & 0xff000000) >> 24; }

  Key leader() const {
    return Key(leader_code(), leader_modifiers());
  }
  void set_leader(int leader_code, int leader_modifiers) {
    data_ = (data_ & 0xffff) | (Make(leader_code, leader_modifiers) << 16);
  }
  void set_leader(Key leader_key) {
    set_leader(leader_key.code(), leader_key.modifiers());
  }

  void Set(int code, int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers);
  }

  void Set(int leader_code, int leader_modifiers, int code, int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers) | (Make(leader_code, leader_modifiers) << 16);
  }

  void Reset() { data_ = 0; }

  bool IsEmpty() const { return data_ == 0; }


  wxString ToString() const;

private:
  int Make(int code, int modifiers) {
    return (code | (modifiers << 8));
  }

private:
  // Encode key stroke in the last 4 bytes of int.
  int data_;
};

inline bool operator==(Key lhs, Key rhs) {
  return lhs.data() == rhs.data();
}
inline bool operator!=(Key lhs, Key rhs) {
  return lhs.data() != rhs.data();
}
inline bool operator<(Key lhs, Key rhs) {
  return lhs.data() < rhs.data();
}
inline bool operator>(Key lhs, Key rhs) {
  return lhs.data() > rhs.data();
}

} } // namespace jil::editor

#endif // JIL_EDITOR_KEY_H_
