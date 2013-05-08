#include "editor/text_line.h"
#include "editor/text_buffer.h"
#include "gtest/gtest.h"

using namespace jil::editor;

const wchar_t* const kEmptyString = L"";

#if EDITOR_USE_SIMPLE_LINE
TEST(TextLine, Wrap) {
}

#else
TEST(TextLine, Constructor) {
  TextLine line1;
  EXPECT_EQ(kEmptyString, line1.AsString());

  TextLine line2(kEmptyString);
  EXPECT_EQ(kEmptyString, line2.AsString());

  TextLine line3(L"AAAA");
  EXPECT_EQ(L"AAAA", line3.AsString());

  TextLine line4(L"AAAAAAAA");
  EXPECT_EQ(L"AAAAAAAA", line4.AsString());

  TextLine line5(L"AAAAAAAABBB");
  EXPECT_EQ(L"AAAAAAAABBB", line5.AsString());
}

TEST(TextLine, AppendChar) {
  TextLine line1;
  line1.Append(L'A');
  EXPECT_EQ(L"A", line1.AsString());

  TextLine line2(L"AAAAAAAA");
  line2.Append(L'B');
  EXPECT_EQ(L"AAAAAAAAB", line2.AsString());
}

TEST(TextLine, InsertCharBegin) {
  TextLine line1(L"AAAA");
  line1.Insert(0, L'B');
  EXPECT_EQ(L"BAAAA", line1.AsString());

  TextLine line2(L"AAAAAAAA");
  line2.Insert(0, L'B');
  EXPECT_EQ(L"BAAAAAAAA", line2.AsString());

  TextLine line3(L"AAAAAAAAA");
  line3.Insert(0, L'B');
  EXPECT_EQ(L"BAAAAAAAAA", line3.AsString());
}

TEST(TextLine, InsertCharEnd) {
  TextLine line(L"AAAA");
  line.Insert(4, L'B');
  EXPECT_EQ(L"AAAAB", line.AsString());

  TextLine line2(L"AAAAAAAA");
  line2.Insert(8, L'B');
  EXPECT_EQ(L"AAAAAAAAB", line2.AsString());

  TextLine line3(L"AAAAAAAAA");
  line3.Insert(9, L'B');
  EXPECT_EQ(L"AAAAAAAAAB", line3.AsString());
}

TEST(TextLine, InsertCharMiddle) {
  TextLine line(L"AAAA");
  line.Insert(2, L'B');
  EXPECT_EQ(L"AABAA", line.AsString());

  TextLine line2(L"AAAAAAAA");
  line2.Insert(4, L'B');
  EXPECT_EQ(L"AAAABAAAA", line2.AsString());

  TextLine line3(L"AAAAAAAAA");
  line3.Insert(8, L'B');
  EXPECT_EQ(L"AAAAAAAABA", line3.AsString());
}

TEST(TextLine, Delete) {
  TextLine line(L"ABCDEFGHIJKL");
  line.Delete(11);
  EXPECT_EQ(L"ABCDEFGHIJK", line.AsString());

  line.Delete(8);
  EXPECT_EQ(L"ABCDEFGHJK", line.AsString());

  line.Delete(7);
  EXPECT_EQ(L"ABCDEFGJK", line.AsString());

  line.Delete(0);
  EXPECT_EQ(L"BCDEFGJK", line.AsString());
}

#endif // EDITOR_USE_SIMPLE_LINE
