#include "editor/notebook.h"
#include <cassert>
#include <memory>
#include "wx/sizer.h"
#include "wx/log.h"
#ifdef __WXMSW__
#include <objidl.h>
#include <gdiplus.h>
#endif // __WXMSW__
#include "wx/graphics.h"
#include "wx/dcgraph.h"
#include "base/foreach.h"
#include "base/math_util.h"
#include "base/compiler_specific.h"
#include "editor/text_extent.h"
#include "editor/tip.h"

namespace jil {\
namespace editor {\

namespace notebook {\

////////////////////////////////////////////////////////////////////////////////

static const int kMarginTop = 5;
static const int kTabPaddingLeft = 5;
static const int kTabPaddingX = 10;
static const int kTabPaddingTop = 7;
static const int kTabPaddingBottom = 7;

const int kFadeAwayChars = 3;

class TabArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TabArea::TabArea(Notebook* notebook, wxWindowID id)
      : wxPanel(notebook, id)
      , notebook_(notebook)
      , tip_handler_(NULL) {
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  }

  virtual ~TabArea() {
    if (tip_handler_ != NULL) {
      PopEventHandler();
      delete tip_handler_;
    }
  }

  // Page header has no focus.
  virtual bool AcceptsFocus() const OVERRIDE { return false; }

  void SetToolTipEx(const wxString& tooltip) {
    if (tip_handler_ == NULL) {
      tip_handler_ = new TipHandler(this);
      tip_handler_->set_start_on_move(true);
      PushEventHandler(tip_handler_);
    }
    tip_handler_->SetTip(tooltip);
  }

protected:
  virtual wxSize DoGetBestSize() const {
    int y = 0;
    GetTextExtent(wxT("T"), NULL, &y, 0, 0);
    return wxSize(-1, y + kMarginTop + kTabPaddingTop + kTabPaddingBottom);
  }

  void OnSize(wxSizeEvent& evt) {
    notebook_->OnTabSize(evt);
  }

  void OnPaint(wxPaintEvent& evt) {
    wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();
#endif

    notebook_->OnTabPaint(dc, evt);
  }

  void OnMouseEvents(wxMouseEvent& evt) {
    notebook_->OnTabMouse(evt);
  }

  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
    // Do nothing.
  }

private:
  Notebook* notebook_;
  TipHandler* tip_handler_;
};

BEGIN_EVENT_TABLE(TabArea, wxPanel)
EVT_SIZE(TabArea::OnSize)
EVT_PAINT(TabArea::OnPaint)
EVT_MOUSE_EVENTS(TabArea::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST(TabArea::OnMouseCaptureLost)
END_EVENT_TABLE()

} // namespace notebook

////////////////////////////////////////////////////////////////////////////////

using namespace notebook;

BEGIN_EVENT_TABLE(Notebook, wxPanel)
END_EVENT_TABLE()

Notebook::Notebook(const SharedTheme& theme)
    : theme_(theme)
    , tab_min_size_(10)
    , tab_default_size_(130)
    , free_size_(0)
    , batch_(false)
    , need_resize_tabs_(false) {
}

bool Notebook::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(BG));

  tab_area_ = new TabArea(this, wxID_ANY);
  if (theme_->GetColor(TAB_AREA_BG).IsOk()) {
    tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));
  }
  if (theme_->GetFont(TAB_FONT).IsOk()) {
    tab_area_->SetFont(theme_->GetFont(TAB_FONT));
  }

  page_sizer_ = new wxBoxSizer(wxVERTICAL);

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(tab_area_, 0, wxEXPAND);
  sizer->Add(page_sizer_, 1, wxEXPAND);
  SetSizer(sizer);

  return true;
}

Notebook::~Notebook() {
  foreach (Tab* tab, tabs_) {
    delete tab;
  }
  tabs_.clear();
}

void Notebook::StartBatch() {
  batch_ = true;
  Freeze();
}

void Notebook::EndBatch() {
  batch_ = false;
  Thaw();

  if (need_resize_tabs_) {
    ResizeTabs(/*refresh=*/true);
    need_resize_tabs_ = false;
  }
}

bool Notebook::AddPage(notebook::IPage* page, bool active/*=false*/) {
  Tab* tab = new Tab(page, CalcTabBestSize(page->NPage_GetLabel()));
  tabs_.push_back(tab);

  // Try to avoid resizing tabs.
  int expected_size = tab->best_size > tab_default_size_ ? tab->best_size : tab_default_size_;
  if (expected_size <= free_size_) {
    tab->size = expected_size;
    free_size_ -= expected_size;
  } else {
    if (!batch_) {
      ResizeTabs();
    } else {
      need_resize_tabs_ = true;
    }
  }

  if (active) {
    ActivatePage(--tabs_.end());
  } else {
    active_age_list_.push_back(tab);
  }

  return true;
}

/*
bool Notebook::RemovePage(size_t index) {
  if (index >= PageCount()) {
    return false; // Invalid index
  }

  IPage* page = GetPage(index);
  if (!page->NPage_Close()) {
    // Failed to close page, stop removing.
    return false;
  }

  RemovePageInfoByIndex(index);

  // The page to remove is active.
  if (index == active_index_) {
    page_sizer_->Detach(page->NPage_AsWindow());
    page->NPage_AsWindow()->Destroy();

    // The page is removed, activate another page.
    if (active_index_ > 0) {
      --active_index_; // Activate previous page
    } else if (PageCount() == 0) { // active_index_ == 0
      // No page left.
      active_index_ = wxNOT_FOUND;
    }
    if (active_index_ < PageCount()) {
      GetActivePage()->NPage_Activate(true);
    }
  } else {
    // If the page to remove is before the active page,
    // the active index will be decreased.
    if (index < active_index_) {
      --active_index_;
    }
  }

  header_panel_->Refresh();
  return true;
}*/

bool Notebook::RemoveActivePage() {
  TabList::iterator it = GetActiveTab();
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool Notebook::RemoveAllPages() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ) {
    Tab* tab = *it;
    if (!tab->page->NPage_Close()) {
      // Failed to close page, stop removing.
      break;
    }
    it = tabs_.erase(it);
    active_age_list_.remove(tab);
    if (tab->active) {
      page_sizer_->Clear(false);
    }
    tab->page->NPage_AsWindow()->Destroy();
    delete tab;
  }

  if (PageCount() == 0) {
    tab_area_->Refresh();
    return true; // All removed
  }

  if (!active_age_list_.front()->active) {
    // The active page is removed, activate another page.
    Tab* tab = active_age_list_.front();
    tab->active = true;
    page_sizer_->Clear(false);
    page_sizer_->Add(tab->page->NPage_AsWindow(), 1, wxEXPAND);
    tab->page->NPage_Activate(true);
  }

  tab_area_->Refresh();
  return false;
}

void Notebook::ActivatePage(IPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      ActivatePage(it);
      return;
    }
  }
}

IPage* Notebook::GetActivePage() const {
  if (active_age_list_.empty()) {
    return NULL;
  }
  if (!active_age_list_.front()->active) {
    return NULL;
  }
  return active_age_list_.front()->page;
}

void Notebook::SwitchPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = active_age_list_.begin();
  ++it;
  ActivatePage((*it)->page);
}

std::vector<notebook::IPage*> Notebook::GetPages() const {
  std::vector<notebook::IPage*> pages;

  foreach (Notebook::Tab* tab, tabs_) {
    pages.push_back(tab->page);
  }

  return pages;
}

void Notebook::ResizeTabs(bool refresh/*=true*/) {
  if (tabs_.empty()) {
    return;
  }

  wxLogDebug("Notebook::ResizeTabs");

  free_size_ = 0; // Reset

  const int client_size = tab_area_->GetClientSize().GetWidth();

  int sum_prev_size = 0;
  int sum_best_size = 0;
  foreach (Tab* tab, tabs_) {
    sum_prev_size += tab->size;
    sum_best_size += tab->best_size;
    // Reset size.
    tab->size = tab->best_size;
  }

  if (sum_best_size < client_size) {
    // Give more size to small tabs.

    TabList small_tabs;
    foreach (Tab* tab, tabs_) {
      if (tab->size < tab_default_size_) {
        small_tabs.push_back(tab);
      }
    }

    int free_size = client_size - sum_best_size;

    while (!small_tabs.empty()) {
      const int avg_free_size = free_size / small_tabs.size();
      free_size %= small_tabs.size();

      if (avg_free_size == 0) {
        // Give 1px to each small tab.
        foreach (Tab* tab, small_tabs) {
          if (free_size == 0) {
            break;
          }
          ++tab->size;
          --free_size;
        }
        break;
      }

      for (TabList::iterator it = small_tabs.begin(); it != small_tabs.end(); ) {
        Tab* tab = *it;
        int needed_size = tab_default_size_ - tab->size;
        if (needed_size > avg_free_size) {
          tab->size += avg_free_size;
          ++it;
        } else {
          // This tab doesn't need that much size.
          tab->size = tab_default_size_;
          // Return extra free size back.
          free_size += avg_free_size - needed_size;
          // This tab is not small any more, remove it from small tab list.
          it = small_tabs.erase(it);
        }
      }
    }

    // Save free size.
    free_size_ = free_size;
    wxLogDebug("ResizeTabs, free_size: %d", free_size_);

  } else { // sum_best_size >= client_size
    // Reduce the size of large tabs.
    int lack_size = sum_best_size - client_size;

    for (int large_size = tab_default_size_; large_size > tab_min_size_ && lack_size > 0; large_size /= 2) {
      TabList large_tabs;
      foreach (Tab* tab, tabs_) {
        if (tab->size > large_size) {
          large_tabs.push_back(tab);
        }
      }

      if (large_tabs.empty()) {
        continue;
      }

      while (!large_tabs.empty()) {
        const int avg_lack_size = lack_size / large_tabs.size();
        lack_size %= large_tabs.size();

        if (avg_lack_size == 0) {
          // Take 1px from first "lack_size" number of large tabs.
          foreach (Tab* tab, large_tabs) {
            if (lack_size == 0) {
              break;
            }
            --tab->size;
            --lack_size;
          }
          break;
        }

        for (TabList::iterator it = large_tabs.begin(); it != large_tabs.end(); ) {
          Tab* tab = *it;
          int givable_size = tab->size - large_size;
          if (givable_size > avg_lack_size) {
            tab->size -= avg_lack_size;
            ++it;
          } else {
            // This tab cannot give that much size. Give all it can give.
            tab->size = large_size;
            // Return extra lack size back.
            lack_size += avg_lack_size - givable_size;
            // This tab is not large any more, remove it from large tab list.
            it = large_tabs.erase(it);
          }
        }
      }
    }
  }

  if (refresh) {
    // Refresh if the size of tabs is changed.
    int sum_size = 0;
    foreach (Tab* tab, tabs_) {
      sum_size += tab->size;
    }
    if (sum_size != sum_prev_size) {
      RefreshTabArea();
    }
  }
}

void Notebook::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
  }
  evt.Skip();
}

#ifdef __WXMSW__

static inline Gdiplus::Color GdiplusColor(const wxColour& wx_color) {
  return Gdiplus::Color(wx_color.Alpha(), wx_color.Red(), wx_color.Green(), wx_color.Blue());
}

static inline Gdiplus::Rect GdiplusRect(const wxRect& wx_rect) {
  return Gdiplus::Rect(wx_rect.x, wx_rect.y, wx_rect.width, wx_rect.height);
}

static Gdiplus::Font* GdiplusNewFont(const wxFont& wx_font) {
  int style = Gdiplus::FontStyleRegular;

  if (wx_font.GetStyle() == wxFONTSTYLE_ITALIC) {
    style |= Gdiplus::FontStyleItalic;
  }
  if (wx_font.GetUnderlined()) {
    style |= Gdiplus::FontStyleUnderline;
  }
  if (wx_font.GetWeight() == wxFONTWEIGHT_BOLD) {
    style |= Gdiplus::FontStyleBold;
  }

  return new Gdiplus::Font(wx_font.GetFaceName().wc_str(),
                           wx_font.GetPointSize(),
                           style,
                           Gdiplus::UnitPoint);
}

#endif // __WXMSW__

void Notebook::OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt) {

#ifdef __WXMAC__
  wxGraphicsContext* gc = dc.GetGraphicsContext();
#else
  wxGCDC gcdc(dc);
  wxGraphicsContext* gc = gcdc.GetGraphicsContext();
#endif // __WXMAC__

  DrawBackground(gc);
  DrawForeground(gc, dc);
}

void Notebook::DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x) {
  const wxRect rect = tab_area_->GetClientRect();

  if (tab->active) {
    gc->SetPen(wxPen(theme_->GetColor(ACTIVE_TAB_BORDER)));
    gc->SetBrush(wxBrush(theme_->GetColor(ACTIVE_TAB_BG)));
  } else {
    gc->SetPen(wxPen(theme_->GetColor(TAB_BORDER)));
    gc->SetBrush(wxBrush(theme_->GetColor(TAB_BG)));
  }

  wxRect tab_rect(x + 1, rect.GetTop() + kMarginTop, tab->size - 1, rect.GetHeight() - kMarginTop + 1);

  wxGraphicsPath border_path = gc->CreatePath();
  border_path.MoveToPoint(tab_rect.GetLeft(), tab_rect.GetBottom());

  //border_path.AddCurveToPoint(tab_rect.GetLeft() + tab_rect.GetHeight() * 0.67, tab_rect.GetBottom(), // Control point 1
  //                            tab_rect.GetLeft() + tab_rect.GetHeight() * 0.67, tab_rect.GetTop(), // Control point 2
  //                            tab_rect.GetLeft() + tab_rect.GetHeight(), tab_rect.GetTop());

  const wxDouble kRadius = 4.0f;
  // Top left corner.
  border_path.AddArc(tab_rect.GetLeft() + kRadius, tab_rect.GetTop() + kRadius, kRadius, base::kRadian180, base::kRadian270, true);
  // Top line.
  border_path.AddLineToPoint(tab_rect.GetRight() - kRadius, tab_rect.GetTop());
  // Top right corner.
  border_path.AddArc(tab_rect.GetRight() - kRadius, tab_rect.GetTop() + kRadius, kRadius, base::kRadian270, base::kRadian0, true);
  // Right line.
  border_path.AddLineToPoint(tab_rect.GetRight(), tab_rect.GetBottom());

  gc->DrawPath(border_path);

  if (!tab->active) {
    gc->StrokeLine(x, rect.GetBottom(), x + tab->size + 1, rect.GetBottom());
  }
}

void Notebook::DrawBackground(wxGraphicsContext* gc) {
  const wxRect rect = tab_area_->GetClientRect();

  int x = rect.GetLeft();

  foreach (Tab* tab, tabs_) {
    DrawTabBackground(tab, gc, x);
    x += tab->size;
  }

  if (x <= rect.GetRight()) {
    gc->StrokeLine(x, rect.GetBottom(), rect.GetRight() + 1, rect.GetBottom());
  }
}

void Notebook::DrawForeground(wxGraphicsContext* gc, wxDC& dc) {
  const wxRect rect = tab_area_->GetClientRect();

  gc->SetFont(tab_area_->GetFont(), theme_->GetColor(ACTIVE_TAB_FG));

#ifdef __WXMSW__
  std::auto_ptr<Gdiplus::Graphics> graphics;
  std::auto_ptr<Gdiplus::Font> gdip_font;
#endif // __WXMSW__

  int x = rect.GetLeft();

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it) {
    Tab* tab = *it;

    const wxRect tab_rect(x + kTabPaddingLeft,
                          rect.GetTop() + kTabPaddingTop + kMarginTop,
                          tab->size - kTabPaddingX,
                          rect.GetHeight() - kTabPaddingTop - kMarginTop);

    if (tab_rect.GetWidth() > 0) {
      if (tab->best_size <= tab->size) {
        // Enough space to display the label completely.
        gc->DrawText(tab->page->NPage_GetLabel(), tab_rect.GetLeft(), tab_rect.GetTop());
      } else {
        // No enough space.
        wxString label = tab->page->NPage_GetLabel();

        // Fit label to the given space.
        label = label.Mid(0, TailorLabel(dc, label, tab_rect.GetWidth()));

        wxDouble fade_x = tab_rect.GetLeft();

        if (label.size() > kFadeAwayChars) {
          wxString label_before = label.Mid(0, label.size() - kFadeAwayChars);
          gc->DrawText(label_before, tab_rect.GetLeft(), tab_rect.GetTop());

          wxDouble label_before_width = 0.0f;
          gc->GetTextExtent(label_before, &label_before_width, NULL, NULL, NULL);
          fade_x += label_before_width;

          label = label.Mid(label.size() - kFadeAwayChars);
        }

        // Fade away the last several characters.

        wxDouble label_width = 0.0f;
        gc->GetTextExtent(label, &label_width, NULL, NULL, NULL);

#ifdef __WXMSW__
        // Lazily create GDI+ objects.
        if (graphics.get() == NULL) {
          graphics.reset(new Gdiplus::Graphics(dc.GetHDC()));
          gdip_font.reset(GdiplusNewFont(tab_area_->GetFont()));
        }

        const wxColour& tab_bg = tab->active ? theme_->GetColor(ACTIVE_TAB_BG) : theme_->GetColor(TAB_BG);
        const wxColour& tab_fg = tab->active ? theme_->GetColor(ACTIVE_TAB_FG) : theme_->GetColor(TAB_FG);
        std::auto_ptr<Gdiplus::Brush> brush(new Gdiplus::LinearGradientBrush(
          GdiplusRect(wxRect(fade_x, tab_rect.GetTop(), std::ceil(label_width), tab_rect.GetHeight())),
          GdiplusColor(tab_fg),
          GdiplusColor(tab_bg),
          Gdiplus::LinearGradientModeHorizontal));

        graphics->DrawString(label.wc_str(*wxConvUI),
                             -1,
                             gdip_font.get(),
                             Gdiplus::PointF(fade_x, tab_rect.GetTop()),
                             Gdiplus::StringFormat::GenericTypographic(),
                             brush.get());
#else
        // TODO: Fade away in other platforms.
        //gc->DrawText(label, fade_x, tab_rect.GetTop());
#endif // __WXMSW__
      }
    }

    // TODO
    // Modified indicator.
    //if (tab->page->NPage_IsModified()) {
    //  dc.SetTextForeground(*wxRED);
    //  dc.DrawText(wxT("*"), x + kTabPaddingX, rect.GetBottom() - kTabPaddingBottom + 1);
    //  dc.SetTextForeground(theme_->>notebook.tab_fg);
    //}

    x += tab->size;
  }
}

void Notebook::OnTabMouse(wxMouseEvent& evt) {
  wxEventType evt_type = evt.GetEventType();
  if (evt_type == wxEVT_LEFT_DOWN) {
    //HandleHeaderMouseLeftDown(evt);
  } else if (evt_type == wxEVT_LEFT_UP) {
    HandleTabMouseLeftUp(evt);
  } else if (evt_type == wxEVT_MOTION) {
    HandleTabMouseMotion(evt);
  } else if (evt_type == wxEVT_RIGHT_DOWN) {
    //HandleHeaderMouseRightDown(evt);
  } else if (evt_type == wxEVT_RIGHT_UP) {
    //HandleHeaderMouseRightUp(evt);
  } else if (evt_type == wxEVT_MIDDLE_DOWN) {
    HandleTabMouseMiddleDown(evt);
  } else if (evt_type == wxEVT_MIDDLE_UP) {
    HandleTabMouseMiddleUp(evt);
  } else if (evt_type == wxEVT_ENTER_WINDOW) {
  } else if (evt_type == wxEVT_LEAVE_WINDOW) {
  }
  evt.Skip();
}

void Notebook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  ActivatePage(GetTabByPos(evt.GetPosition().x));
}

void Notebook::HandleTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }
}

void Notebook::HandleTabMouseMiddleUp(wxMouseEvent& evt) {
  if (tab_area_->HasCapture()) {
    tab_area_->ReleaseMouse();
  }

  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  RemovePage(it);
}

// Update tooltip on mouse motion.
void Notebook::HandleTabMouseMotion(wxMouseEvent& evt) {
  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    tab_area_->SetToolTipEx(wxEmptyString);
    return;
  }

  Tab* tab = *it;
  tab_area_->SetToolTipEx(tab->page->NPage_GetLabel());
}

Notebook::TabList::iterator Notebook::GetTabByPos(int client_x) {
  int x = 0;

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if (client_x >= x && client_x < x + (*it)->size) {
      return it;
    }

    x += (*it)->size;
  }

  return tabs_.end();
}

Notebook::Tab* Notebook::GetTabByWindow(wxWindow* window, size_t* index) {
  size_t i = 0;

  foreach (Tab* tab, tabs_) {
    if (tab->page->NPage_AsWindow() == window) {
      if (index != NULL) {
        *index = i;
      }
      return tab;
    }

    ++i;
  }

  return NULL;
}

void Notebook::ActivatePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return; // Already active
  }

  Freeze();

  // Deactivate previous active page.
  TabList::iterator active_it = GetActiveTab();
  if (active_it != tabs_.end()) {
    (*active_it)->active = false;
    (*active_it)->page->NPage_Activate(false);
    page_sizer_->Clear(false);
  }

  // Activate new page.
  (*it)->active = true;
  page_sizer_->Add((*it)->page->NPage_AsWindow(), 1, wxEXPAND);
  (*it)->page->NPage_Activate(true);

  // Update active age list.
  active_age_list_.remove(tab);
  active_age_list_.push_front(tab);

  Layout();
  Thaw();
  tab_area_->Refresh();
}

bool Notebook::RemovePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  if (!tab->page->NPage_Close()) {
    // Failed to close page, stop removing.
    return false;
  }

  tabs_.erase(it);
  active_age_list_.remove(tab);

  // The page to remove is active.
  if (tab->active) {
    page_sizer_->Clear(false);
    tab->page->NPage_AsWindow()->Destroy();

    if (PageCount() > 0) {
      // Activate another page.
      Tab* active_page_info = active_age_list_.front();
      active_page_info->active = true;
      page_sizer_->Add(active_page_info->page->NPage_AsWindow(), 1, wxEXPAND);
      active_page_info->page->NPage_Activate(true);
    }
  }

  delete tab;

  // The tab is removed, more space is available, resize the left tabs.
  ResizeTabs(/*refresh=*/false);
  tab_area_->Refresh();

  return true;
}

Notebook::TabList::iterator Notebook::GetActiveTab() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

int Notebook::CalcTabBestSize(const wxString& label) const {
  std::auto_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(tab_area_));
  gc->SetFont(tab_area_->GetFont(), *wxWHITE);

  wxDouble label_size = 0.0f;
  gc->GetTextExtent(label, &label_size, NULL, NULL, NULL);

  return static_cast<int>(std::ceil(label_size)) + kTabPaddingX;
}

void Notebook::RefreshTabArea() {
  tab_area_->Refresh();
}

} } // namespace jil::editor
