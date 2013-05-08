#ifndef JIL_SESSION_H_
#define JIL_SESSION_H_
#pragma once

// Search history, recent opened files, last cursor positions, top window states, etc.

#include "wx/gdicmn.h"

namespace jil {\

class Session {
public:
  Session();

  bool Load(const wxString& file);
  bool Save(const wxString& file);

  // Book frame states:

  wxRect book_frame_rect() const { return book_frame_rect_; }
  void set_book_frame_rect(const wxRect& rect) { book_frame_rect_ = rect; }

  bool book_frame_maximized() const { return book_frame_maximized_; }
  void set_book_frame_maximized(bool maximized) { book_frame_maximized_ = maximized; }

private:
  wxRect book_frame_rect_;
  bool book_frame_maximized_;
};

} // namespace jil

#endif // JIL_SESSION_H_
