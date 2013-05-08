#include <memory>
#include "editor/text_buffer.h"
#include "base/compiler_specific.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(TextBuffer, SetText_Empty) {
  //std::auto_ptr<TextBuffer> buffer = TextBuffer::Create(NULL, wxEmptyString);

  //buffer->SetText(_T(""));

  //EXPECT_EQ(1, buffer->LineCount());
}
/*
TEST(TextBuffer, LineEndingsOnly) {
  StringVector lines;

  TextBuffer(_T("\r"), lines);
  EXPECT_EQ(1, lines.size());
  EXPECT_TRUE(lines[0].empty());

  lines.clear();
  TextBuffer(_T("\n"), lines);
  EXPECT_EQ(1, lines.size());
  EXPECT_TRUE(lines[0].empty());

  lines.clear();
  TextBuffer(_T("\r\n"), lines);
  EXPECT_EQ(1, lines.size());
  EXPECT_TRUE(lines[0].empty());

  lines.clear();
  TextBuffer(_T("\r\r"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_TRUE(lines[0].empty());
  EXPECT_TRUE(lines[1].empty());

  lines.clear();
  TextBuffer(_T("\n\n"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_TRUE(lines[0].empty());
  EXPECT_TRUE(lines[1].empty());

  lines.clear();
  TextBuffer(_T("\r\n\r\n"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_TRUE(lines[0].empty());
  EXPECT_TRUE(lines[1].empty());
}

TEST(TextBuffer, NoLineEndingsAtLast) {
  StringVector lines;

  TextBuffer(_T("abc"), lines);
  EXPECT_EQ(1, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);

  lines.clear();
  TextBuffer(_T("abc\nde"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);

  lines.clear();
  TextBuffer(_T("abc\rde"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);

  lines.clear();
  TextBuffer(_T("abc\r\nde"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);
}

TEST(TextBuffer, NormalCases) {
  StringVector lines;

  TextBuffer(_T("abc\nde\n"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);

  lines.clear();
  TextBuffer(_T("abc\rde\r"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);

  lines.clear();
  TextBuffer(_T("abc\r\nde\r\n"), lines);
  EXPECT_EQ(2, lines.size());
  EXPECT_EQ(_T("abc"), lines[0]);
  EXPECT_EQ(_T("de"), lines[1]);
}
*/