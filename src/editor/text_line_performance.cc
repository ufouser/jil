#include "boost/pool/pool.hpp"
#include "boost/pool/object_pool.hpp"
#include "base/foreach.h"
#include "base/performance_util.h"
#include "editor/line_piece.h"
#include "editor/text_line.h"

using namespace easyhs::editor;

////////////////////////////////////////////////////////////////////////////////
// Test the performance of boost::pool.

// Test boost::pool.
template <size_t count>
void TestPool() {
  boost::pool<> p(sizeof(LinePiece));
  for (size_t i = 0; i < count; ++i) {
    LinePiece* piece = static_cast<LinePiece*>(p.malloc());
    p.free(piece);
  }
}

// Test boost::object_pool.
template <size_t count>
void TestObjectPool() {
  boost::object_pool<LinePiece> p;
  for (size_t i = 0; i < count; ++i) {
    LinePiece* piece = p.malloc();
  }
}

// Test using no pool.
template <size_t count>
void TestNoPool() {
  for (size_t i = 0; i < count; ++i) {
    LinePiece* piece = new LinePiece;
    delete piece;
  }
}

////////////////////////////////////////////////////////////////////////////////

template <size_t repeat, size_t count>
void TextLine_Append() {
  for (size_t i = 0; i < repeat; ++i) {
    TextLine line;
    for (size_t i = 0; i < count; ++i) {
      line.Append(L'A');
    }
  }
}

template <size_t repeat, size_t count>
void String_Append() {
  for (size_t i = 0; i < repeat; ++i) {
    std::wstring line;
    for (size_t i = 0; i < count; ++i) {
      line.append(1, L'A');
    }
  }
}

template <size_t repeat, size_t count>
void TextLine_InsertBegin() {
  for (size_t i = 0; i < repeat; ++i) {
    TextLine line;
    for (size_t i = 0; i < count; ++i) {
      line.Insert(0, L'A');
    }
  }
}

template <size_t repeat, size_t count>
void String_InsertBegin() {
  for (size_t i = 0; i < repeat; ++i) {
    std::wstring line;
    for (size_t i = 0; i < count; ++i) {
      line.insert(0, 1, L'A');
    }
  }
}

template <size_t repeat, size_t count>
void TextLine_Delete() {
  for (size_t i = 0; i < repeat; ++i) {
    TextLine line;
    for (size_t i = 0; i < count; ++i) {
      line.Insert(0, L'A');
    }
    for (size_t i = 0; i < count; ++i) {
      line.Delete(0);
    }
  }
}

template <size_t repeat, size_t count>
void String_Delete() {
  for (size_t i = 0; i < repeat; ++i) {
    std::wstring line;
    for (size_t i = 0; i < count; ++i) {
      line.insert(0, 1, L'A');
    }
    for (size_t i = 0; i < count; ++i) {
      line.erase(0, 1);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

using base::TimeIt;

void TestPoolPerformance() {
  const size_t kCount = 10000000;

  TimeIt(TestPool<kCount>, "TestPool");
  TimeIt(TestObjectPool<kCount>, "TestObjectPool");
  TimeIt(TestNoPool<kCount>, "TestNoPool");
}

void TestTextLinePerformance() {
  const size_t kRepeat = 10000;
  const size_t kCount = 80;

  TimeIt(TextLine_Append<kRepeat, kCount>, "TextLine_Append");
  TimeIt(String_Append<kRepeat, kCount>, "String_Append");

  TimeIt(TextLine_InsertBegin<kRepeat, kCount>, "TextLine_InsertBegin");
  TimeIt(String_InsertBegin<kRepeat, kCount>, "String_InsertBegin");

  TimeIt(TextLine_Delete<kRepeat, kCount>, "TextLine_Delete");
  TimeIt(String_Delete<kRepeat, kCount>, "String_Delete");
}

int main() {
  TestPoolPerformance();
  TestTextLinePerformance();
}
