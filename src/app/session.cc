#include "app/session.h"
#include "app/config.h"

namespace jil {\

// Setting names.
const char* const kBookFrame = "book_frame";
const char* const kWindow = "window";
const char* const kRect = "rect";
const char* const kMaximized = "maximized";

Session::Session()
    : book_frame_maximized_(false) {
}

bool Session::Load(const wxString& file) {
  Config config;
  if (!config.Load(file)) {
    return false;
  }

  Setting window_setting = config.root().Get(kWindow, Setting::kGroup);

  Setting main_frame_setting = window_setting.Get(kBookFrame, Setting::kGroup);
  main_frame_setting.GetRect(kRect, book_frame_rect_);
  book_frame_maximized_ = main_frame_setting.GetBool(kMaximized);

  return true;
}

bool Session::Save(const wxString& file) {
  Config config;
  Setting window_setting = config.root().Get(kWindow, Setting::kGroup);

  Setting main_frame_setting = window_setting.Get(kBookFrame, Setting::kGroup);
  if (!book_frame_rect_.IsEmpty()) {
    main_frame_setting.SetRect(kRect, book_frame_rect_);
  }
  main_frame_setting.SetBool(kMaximized, book_frame_maximized_);

  return config.Save(file);
}

} // namespace jil
