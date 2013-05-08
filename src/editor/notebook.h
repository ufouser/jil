#ifndef JIL_EDITOR_NOTEBOOK_H_
#define JIL_EDITOR_NOTEBOOK_H_
#pragma once

#include "wx/panel.h"
#include <list>
#include <vector>
#include "wx/dcbuffer.h"
#include "editor/theme.h"

class wxSizer;
#ifndef __WXMAC__
class wxMemoryDC;
#endif // __WXMAC__
class wxGraphicsContext;

namespace jil {\
namespace editor {\

namespace notebook {\
class TabArea;

// Notebook page interface.
class IPage {
public:
  virtual ~IPage() {}

  // Add prefix "NPage" to avoid naming conflict.
  virtual wxWindow* NPage_AsWindow() = 0;

  virtual wxString NPage_GetLabel() const = 0;

  virtual bool NPage_Activate(bool active) = 0;
  virtual bool NPage_IsActive() const = 0;

  virtual bool NPage_IsModified() const = 0;

  // Note: Don't destroy the window since notebook will destroy it.
  virtual bool NPage_Close() = 0;
};

} // namespace notebook

class Notebook : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG = 0, // The whole notebook background

    TAB_AREA_BG, // The tab area background

    // Tab item colors:
    TAB_FG,
    ACTIVE_TAB_FG,
    TAB_BG,
    ACTIVE_TAB_BG,
    TAB_BORDER,
    ACTIVE_TAB_BORDER,

    COLOR_COUNT
  };

  enum FontId {
    TAB_FONT = 0, // Tab text font
    FONT_COUNT
  };

protected:
  class Tab {
  public:
    Tab(notebook::IPage* page_, int best_size_, bool active_ = false, bool modified_ = false)
        : page(page_)
        , best_size(best_size_)
        , size(best_size_)
        , active(active_)
        , modified(modified_) {
    }

    notebook::IPage* page;

    // Tab size.
    int best_size; // Size fitting the label
    int size; // Display size

    bool active;
    bool modified; // TODO
  };

  typedef std::list<Tab*> TabList;

public:
  Notebook(const SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~Notebook();

  // Set batch flag to avoid unecessary resizing tabs and refresh.
  // Example:
  //   notebook->StartBatch();
  //   notebook->AddPage(...);
  //   notebook->AddPage(...);
  //   ...
  //   notebook->EndBatch();
  void StartBatch();
  void EndBatch();

  virtual bool AddPage(notebook::IPage* page, bool active = false);
  bool RemoveActivePage();
  bool RemoveAllPages();

  size_t PageCount() const { return tabs_.size(); }

  void ActivatePage(notebook::IPage* page);
  notebook::IPage* GetActivePage() const;

  void SwitchPage();

  std::vector<notebook::IPage*> GetPages() const;

  // Recalculate the size for each tab.
  // Don't refresh even the tab size changes if @refresh is false.
  void ResizeTabs(bool refresh = true);

protected:
  // Tab area event handlers.
  friend class notebook::TabArea;
  void OnTabSize(wxSizeEvent& evt);

  void OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt);
  void DrawBackground(wxGraphicsContext* gc);
  void DrawForeground(wxGraphicsContext* gc, wxDC& dc);
  // Draw background for one given tab.
  void DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x);

  void OnTabMouse(wxMouseEvent& evt);
  void HandleTabMouseLeftUp(wxMouseEvent& evt);
  void HandleTabMouseMiddleDown(wxMouseEvent& evt);
  void HandleTabMouseMiddleUp(wxMouseEvent& evt);
  void HandleTabMouseMotion(wxMouseEvent& evt);

  TabList::iterator GetTabByPos(int client_x);
  Tab* GetTabByWindow(wxWindow* window, size_t* index = NULL);

  void ActivatePage(TabList::iterator it);
  bool RemovePage(TabList::iterator it);

  TabList::iterator GetActiveTab();

  int CalcTabBestSize(const wxString& label) const;

  void RefreshTabArea();

protected:
  SharedTheme theme_;

  int tab_min_size_;
  int tab_default_size_;

  // Tab area free/available size.
  int free_size_;

  notebook::TabArea* tab_area_;
  wxSizer* page_sizer_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front whenever it's activated.
  TabList active_age_list_;

  bool batch_;
  bool need_resize_tabs_;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_NOTEBOOK_H_
