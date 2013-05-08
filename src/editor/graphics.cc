#include "editor/graphics.h"
#include "wx/graphics.h"

namespace jil {\
namespace editor {\

Graphics::Graphics(wxGraphicsContext* gc)
    : gc_(gc) {
}

Graphics::~Graphics() {

}

void Graphics::SetFont(const wxFont& font, const wxColour& color) {

}

void Graphics::SetBrush(const wxBrush& brush) {

}

void Graphics::SetPen(const wxPen& pen) {

}

void Graphics::DrawText(const wxString& text, int x, int y) {

}

void Graphics::DrawLine(int x1, int y1, int x2, int y2) {

}

void Graphics::DrawRectangle(int x, int y, int h, int w) {

}


} } // namespace jil::editor
