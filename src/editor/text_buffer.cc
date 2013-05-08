#include "editor/text_buffer.h"
#include <cassert>
#include <utility>
#include "wx/log.h"
#include "wx/filesys.h"
#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"
#include "base/compiler_specific.h"
#include "base/foreach.h"
#include "base/string_util.h"
#include "editor/file_type.h"
#include "editor/edit_action.h"
#include "editor/seeker.h"
#include "editor/file_io.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

static
bool IsOpenBracket(wchar_t c) {
  return c == L'(' ||
         c == L'{' ||
         c == L'[' ||
         c == L'<';
}

static
bool IsCloseBracket(wchar_t c) {
  return c == L')' ||
         c == L'}' ||
         c == L']' ||
         c == L'>';
}

enum Bracket {
  kParenthesis = 0, // ()
  kBrace, // {}
  kSquareBracket, // []
  kAngleBracket, // <>
  kBracketCount,
  kNoBracket = kBracketCount
};

static
Bracket CheckOpenBracket(wchar_t c) {
  switch (c) {
    case L'(':
      return kParenthesis;
    case L'{':
      return kBrace;
    case L'[':
      return kSquareBracket;
    case L'<':
      return kAngleBracket;
    default:
      return kNoBracket;
  }
}

static
Bracket CheckCloseBracket(wchar_t c) {
  switch (c) {
    case L')':
      return kParenthesis;
    case L'}':
      return kBrace;
    case L']':
      return kSquareBracket;
    case L'>':
      return kAngleBracket;
    default:
      return kNoBracket;
  }
}

static
bool IsBracketPair(wchar_t open_bracket, wchar_t close_bracket) {
  if (open_bracket == L'(') {
    return close_bracket == L')';
  }
  if (open_bracket == L'{') {
    return close_bracket == L'}';
  }
  if (open_bracket == L'[') {
    return close_bracket == L']';
  }
  if (open_bracket == L'<') {
    return close_bracket == L'>';
  }
  if (open_bracket == L'"') {
    return close_bracket == L'"';
  }
  if (open_bracket == L'\'') {
    return close_bracket == L'\'';
  }
  return false;
}

namespace {

FileFormat CheckEol(const std::wstring& text, size_t* first_line_size) {
  const size_t text_size = text.size();

  FileFormat eol = FF_NONE;

  size_t i = 0;
  for (; i < text_size; ++i) {
    if (text[i] == L'\r') {
      if (i + 1 < text_size && text[i + 1] == L'\n') {
        eol = FF_WIN;
      } else {
        eol = FF_MAC;
      }
      break;
    } else if (text[i] == L'\n') {
      eol = FF_UNIX;
      break;
    }
  }

  if (first_line_size != NULL && eol != FF_NONE) {
    *first_line_size = i;
  }

  return eol;
}

/* Usage:
std::wstring text(L"a\nb\n\nc\n");
size_t i = 0;
size_t count = 0;
size_t step = 0;
while (SplitLines(text, i, &count, &step)) {
  text.substr(i, count);
  i += step;
}
*/
bool SplitLines(const std::wstring& text, size_t i, size_t* count, size_t* step) {
  const size_t text_size = text.size();
  if (i >= text_size) {
    return false;
  }

  size_t k = i;
  for (; k < text_size; ++k) {
    if (text[k] == L'\r' || text[k] == L'\n') {
      break;
    }
  }

  *count = k - i;
  *step = *count + 1;

  // If we break for '\r', the step need to take another '\n' into account.
  if (k + 1 < text_size) {
    if (text[k] == L'\r' && text[k + 1] == L'\n') {
      ++(*step);
    }
  }
  return true;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

// If wxWidgets pre-defines the conv, reuse it and don't delete it.
// Example: reuse wxConvISO8859_1, don't "new wxCSConv(wxFONTENCODING_ISO8859_1)".
static wxMBConv* GetCsConv(const std::string& charset, bool& need_delete) {
  need_delete = true;
  wxMBConv* conv = NULL;
  size_t bom_size = 0;

  if (charset == ASCII) {
    conv = &wxConvISO8859_1;
    need_delete = false;
  } else if (charset == UTF_8) {
    conv = &wxConvUTF8;
    need_delete = false;
  } else if (charset == UTF_8_BOM) {
    bom_size = 3;
    conv = &wxConvUTF8;
    need_delete = false;
  } else if (charset == UTF_16BE) {
    bom_size = 2;
    conv = new wxMBConvUTF16BE;
  } else if (charset == UTF_16LE) {
    bom_size = 2;
    conv = new wxMBConvUTF16LE;
  } else if (charset == GB18030) { // TODO
    conv = new wxCSConv(wxFONTENCODING_GB2312);
  } else if (charset == BIG5) {
    conv = new wxCSConv(wxFONTENCODING_BIG5);
  } else if (charset == SHIFT_JIS) {
    conv = new wxCSConv(wxFONTENCODING_SHIFT_JIS);
  } else if (charset == EUC_JP) {
    conv = new wxCSConv(wxFONTENCODING_EUC_JP);
  } else if (charset == KOI8_R) {
    // wxFONTENCODING_KOI8 means KOI8-R, Russian.
    conv = new wxCSConv(wxFONTENCODING_KOI8);
  } else if (charset == ISO_8859_2) {
    conv = new wxCSConv(wxFONTENCODING_ISO8859_2);
  } else if (charset == ISO_8859_5) { // Cyrillic
    conv = new wxCSConv(wxFONTENCODING_ISO8859_5);
  } else if (charset == ISO_8859_7) { // Greek
    conv = new wxCSConv(wxFONTENCODING_ISO8859_7);
  } else if (charset == TIS_620) {
    conv = new wxCSConv(wxFONTENCODING_ISO8859_11);
  } else if (charset == WINDOWS_1250) {
    conv = new wxCSConv(wxFONTENCODING_CP1250);
  } else if (charset == WINDOWS_1251) {
    conv = new wxCSConv(wxFONTENCODING_CP1251);
  } else if (charset == WINDOWS_1253) {
    conv = new wxCSConv(wxFONTENCODING_CP1253);
  } else if (charset == X_MAC_CYRILLIC) {
    conv = new wxCSConv(wxFONTENCODING_MACCYRILLIC);
  }

  return conv;
}

////////////////////////////////////////////////////////////////////////////////

// Charset detector using uchardet from Mozilla.
// See http://mxr.mozilla.org/mozilla/source/extensions/universalchardet/src/
class CharDetector : public nsUniversalDetector {
public:
  CharDetector(int lang_filter)
      : nsUniversalDetector(lang_filter) {
  }

  virtual ~CharDetector() {
  }

  const std::string& charset() const { return charset_; }

  bool PureAscii() const { return mInputState == ePureAscii; }

protected:
  virtual void Report(const char* charset) OVERRIDE {
    charset_ = base::StringToLowerCopy(charset);
  }

private:
  std::string charset_;
};

////////////////////////////////////////////////////////////////////////////////

// static
TextBuffer* TextBuffer::Create(FileType* file_type, const wxString& file_encoding) {
  TextBuffer* buffer = new TextBuffer(file_type);

  buffer->set_file_encoding(file_encoding);

  // A new file has an empty line.
  buffer->AppendLine();

  return buffer;
}

// static
TextBuffer* TextBuffer::Create(const wxFileName& file_name_object,
                               FileType* file_type,
                               int cjk_filters,
                               const wxString& file_encoding) {
  std::string bytes;
  int result = ReadBytes(file_name_object.GetFullPath(), bytes);
  if (result != 0) {
    // IO Error or size limit is exceeded. (TODO)
    return NULL;
  }

  if (bytes.empty()) {
    // The file is empty.
    TextBuffer* buffer = Create(file_type, file_encoding);
    buffer->set_file_name_object(file_name_object);
    return buffer;
  }

  // Detect charset (i.e., file encoding).
  CharDetector char_detector(cjk_filters);
  char_detector.HandleData(bytes.c_str(), bytes.size());
  char_detector.DataEnd();

  std::string charset = char_detector.charset();
  if (charset.empty()) {
    if (char_detector.PureAscii()) {
      charset = ASCII; // TODO
    } else {
      // Failed to detect the file encoding!
      // TODO:
      // Return error code.
      // Should allow user to choose the file encoding manually.
      return NULL;
    }
  }

  bool conv_need_delete = true;
  wxMBConv* conv = GetCsConv(charset, conv_need_delete);
  if (conv == NULL) {
    return NULL; // TODO
  }

  size_t bom_size = 0;
  if (charset == UTF_8_BOM) {
    bom_size = 3;
  } else if (charset == UTF_16BE || charset == UTF_16LE) {
    bom_size = 2;
  }

  std::wstring text;
  wxString encoding = wxString::FromAscii(charset.c_str());

  // Get the size in wchar_t.
  size_t wlen = conv->ToWChar(NULL, 0, bytes.c_str() + bom_size, bytes.size() - bom_size);
  if (wlen != wxCONV_FAILED && wlen > 0) {
    text.resize(wlen);
    conv->ToWChar(&text[0], wlen, bytes.c_str() + bom_size, bytes.size() - bom_size);
    if (text[wlen - 1] == L'\0') {
      text.erase(wlen - 1);
    }
  } else {
    // TODO: Error
  }

  if (conv_need_delete) {
    delete conv;
    conv = NULL;
  }

  TextBuffer* buffer = NULL;
  if (text.empty()) {
    // The file is empty with only BOM bytes?
    buffer = Create(file_type, encoding);
  } else {
    buffer = new TextBuffer(file_type);
    buffer->set_file_encoding(encoding);
    buffer->SetText(text);
  }
  buffer->set_file_name_object(file_name_object);

  return buffer;
}

// static
TextBuffer* TextBuffer::Create(const std::wstring& text,
                               FileType* file_type,
                               const wxString& file_encoding) {
  TextBuffer* buffer = NULL;
  if (text.empty()) {
    // The file is empty with only BOM bytes?
    buffer = Create(file_type, file_encoding);
  } else {
    buffer = new TextBuffer(file_type);
    buffer->set_file_encoding(file_encoding);
    buffer->SetText(text);
  }
  return buffer;
}

ErrorCode TextBuffer::SaveFile() {
  // VC++ needs "this->" if variable has the same name as the function.
  wxString file_path_name = this->file_path_name();
  assert(!file_path_name.empty());

  const std::wstring& eol = GetEOL(file_format_);
  std::wstring text = lines_[0]->data();
  for (size_t i = 1; i < lines_.size(); ++i) {
    text += eol;
    text += lines_[i]->data();
  }

  // Convert back to bytes with the original encoding.
  std::string charset = file_encoding_.ToAscii().data();

  const char* bom = NULL;
  if (charset == UTF_8_BOM) {
    bom = UTF_8_BOM_BYTES;
  } else if (charset == UTF_16BE) {
    bom = UTF_16BE_BOM_BYTES;
  } else if (charset == UTF_16LE) {
    bom = UTF_16LE_BOM_BYTES;
  }

  const char* new_file_encoding = NULL;

  bool conv_need_delete = true;
  wxMBConv* conv = GetCsConv(charset, conv_need_delete);
  if (conv == NULL) {
    // Save in UTF-8 instead.
    conv = &wxConvUTF8;
    conv_need_delete = false;
    bom = NULL;
    new_file_encoding = UTF_8;
  }

  // Get the size in bytes.
  // FIXME: Cannot use std::auto_ptr! Strange!
  size_t count = conv->FromWChar(NULL, 0, text.c_str(), text.size());
  if (count == wxCONV_FAILED) {
    // Can't save in original encoding.
    // Example: Original encoding is ASCII, but non-ASCII characters are added.
    // Save in UTF-8 instead.
    if (conv_need_delete) {
      delete conv;
    }
    conv = &wxConvUTF8;
    conv_need_delete = false;

    count = conv->FromWChar(NULL, 0, text.c_str(), text.size());
    if (count == wxCONV_FAILED) {
      // Still failed!
      if (conv_need_delete) {
        delete conv;
        conv = NULL;
      }
      return kEncodingError;
    }

    bom = NULL;
    new_file_encoding = UTF_8;
  }

  std::string bytes;
  bytes.resize(count);

  conv->FromWChar(&bytes[0], count, &text[0], text.size());
  if (bytes[count - 1] == '\0') {
    --count;
  }

  if (conv_need_delete) {
    delete conv;
    conv = NULL;
  }

  int save_result = SaveBytes(file_path_name, bom, bytes);

  if (save_result != 0) {
    return kIOError;
  }

  if (new_file_encoding != NULL) {
    file_encoding_ = new_file_encoding;
    NotifyEncodingChange();
  }

  SaveUndoActions();

  NotifySaved();

  return kNoError;
}

TextBuffer::TextBuffer(FileType* file_type)
    : file_type_(file_type)
    , file_format_(FF_DEFAULT)
    , read_only_(false)
    , deleted_(false)
    , line_id_(0)
    , notify_frozen_(false)
    , last_saved_undo_count_(0) {
}

TextBuffer::~TextBuffer() {
  ClearEditActions();

  foreach (TextLine* line, lines_) {
    delete line;
  }
  lines_.clear();
}

wxString TextBuffer::file_path_name() const {
  return file_name_object_.GetFullPath();
}

void TextBuffer::set_file_path_name(const wxString& file_path_name) {
  file_name_object_ = wxFileName(file_path_name);
  NotifyFileNameChange();
}

wxString TextBuffer::file_name() const {
  return file_name_object_.GetFullName();
}

wxString TextBuffer::file_path() const {
  return file_name_object_.GetPath();
}

//------------------------------------------------------------------------------

bool TextBuffer::new_created() const {
  return file_name().empty();
}

bool TextBuffer::modified() const {
  if (saved_undo_edit_actions_.size() != last_saved_undo_count_) {
    return true;
  }
  if (!undo_edit_actions_.empty()) {
    return true;
  }
  if (!temp_insert_char_actions_.empty()) {
    return true;
  }
  return false;
}

bool TextBuffer::read_only() const {
  return read_only_;
}

bool TextBuffer::deleted() const {
  return deleted_;
}

//------------------------------------------------------------------------------

Coord TextBuffer::LineCount() const {
  return CoordCast(lines_.size());
}

TextLine& TextBuffer::GetLine(Coord ln) {
  assert(ln > 0 && ln <= LineCount());
  return *lines_[ln - 1];
}

const TextLine& TextBuffer::GetLine(Coord ln) const {
  assert(ln > 0 && ln <= LineCount());
  return *lines_[ln - 1];
}

wchar_t TextBuffer::GetChar(const TextPoint& point) const {
  assert(point.y > 0 && point.y <= LineCount());
  return GetLine(point.y).GetChar(point.x);
}

size_t TextBuffer::GetLineId(Coord ln) const {
  return GetLine(ln).id();
}

Coord TextBuffer::LineLength(Coord ln) const {
  return GetLine(ln).Length();
}

std::wstring TextBuffer::GetText(const TextRange& range) const {
  if (range.first_point().y == range.last_point().y) {
    const std::wstring& line_data = GetLine(range.first_point().y).data();
    return line_data.substr(range.first_point().x, range.last_point().x - range.first_point().x);
  }

  std::wstring text;

  // First line.
  const std::wstring& line_data = GetLine(range.first_point().y).data();
  text = line_data.substr(range.first_point().x, std::wstring::npos);
  text += LF;

  // Middle lines.
  for (Coord y = range.first_point().y + 1; y < range.last_point().y; ++y) {
    text += GetLine(y).data() + LF;
  }

  // Last line.
  if (range.last_point().x > 0) {
    text += GetLine(range.last_point().y).data().substr(0, range.last_point().x);
  }

  return text;
}

// Note: Don't have to notify since there shouldn't be any listeners when
// SetText() is called.
void TextBuffer::SetText(const std::wstring& text) {
  lines_.clear();
  line_length_table_.clear();

  size_t i = 0;
  size_t first_line_size = 0;
  FileFormat ff = CheckEol(text, &first_line_size);
  if (ff != FF_NONE) {
    // The file has more than one line.
    // Add the first line here, add the left lines later.
    AppendLine(text.substr(0, first_line_size));
    i += first_line_size; // Skip the first line.
    i += ff == FF_WIN ? 2 : 1; // Skip the EOL

    // Overwrite the default file format.
    file_format_ = ff;
  }

  size_t count = 0;
  size_t step = 0;
  while (SplitLines(text, i, &count, &step)) {
    AppendLine(text.substr(i, count));
    i += step;
  }

  // If text ends with EOL, append an extra empty line.
  wchar_t last_char = text[text.size() - 1];
  if (last_char == LF || last_char == CR) {
    AppendLine();
  }
}

TextPoint TextBuffer::InsertChar(const TextPoint& point, wchar_t c) {
  if (c == LF) {
    TextLine& line = GetLine(point.y);
    TextLine* new_line = NULL;
    if (point.x == line.Length()) {
      new_line = new TextLine(NewLineId());
    } else {
      RemoveLineLength(line.Length());
      new_line = line.Split(point.x, NewLineId());
      AddLineLength(line.Length());
      Notify(kLineUpdated, ChangeData(point.y));
    }
    InsertLine(point.y, new_line);
    Notify(kLineAdded, ChangeData(point.y + 1));
    return TextPoint(0, point.y + 1);
  } else {
    TextLine& line = GetLine(point.y);
    RemoveLineLength(line.Length());
    line.InsertChar(point.x, c);
    AddLineLength(line.Length());
    Notify(kLineUpdated, ChangeData(point.y));
    return TextPoint(point.x + 1, point.y);
  }
}

void TextBuffer::DeleteChar(const TextPoint& point, wchar_t* c) {
  assert(point.x <= LineLength(point.y));

  if (point.x == LineLength(point.y)) {
    // Delete the line ending.
    if (point.y < LineCount()) {
      TextLine& line = GetLine(point.y);
      TextLine& next_line = GetLine(point.y + 1);

      RemoveLineLength(next_line.Length());

      if (next_line.Length() > 0) {
        RemoveLineLength(line.Length());
        line.Append(next_line.data());
        AddLineLength(line.Length());
        Notify(kLineUpdated, ChangeData(point.y));
      }

      lines_.erase(lines_.begin() + point.y);
      Notify(kLineDeleted, ChangeData(point.y + 1));

      if (c != NULL) {
        *c = LF;
      }
    } else {
      // Don't delete the line ending of the last line.
      if (c != NULL) {
        *c = kNilChar;
      }
    }
  } else {
    TextLine& line = GetLine(point.y);
    RemoveLineLength(line.Length());
    line.DeleteChar(point.x, c);
    AddLineLength(line.Length());
    Notify(kLineUpdated, ChangeData(point.y));
  }
}

TextPoint TextBuffer::InsertString(const TextPoint& point, const std::wstring& str) {
  TextLine& line = GetLine(point.y);
  RemoveLineLength(line.Length());
  line.InsertString(point.x, str);
  AddLineLength(line.Length());
  Notify(kLineUpdated, ChangeData(point.y));
  return TextPoint(point.x + str.size(), point.y);
}

void TextBuffer::DeleteString(const TextPoint& point, size_t count, std::wstring* str) {
  TextLine& line = GetLine(point.y);
  RemoveLineLength(line.Length());
  line.DeleteString(point.x, count, str);
  AddLineLength(line.Length());
  Notify(kLineUpdated, ChangeData(point.y));
}

void TextBuffer::InsertLine(Coord ln, const std::wstring& line_data) {
  assert(ln > 0 && ln <= LineCount() + 1);
  InsertLine(ln - 1, new TextLine(NewLineId(), line_data));
  Notify(kLineAdded, ChangeData(ln));
}

void TextBuffer::DeleteLine(Coord ln, std::wstring* line_data) {
  assert(ln > 0 && ln <= LineCount());

  if (LineCount() == 1) {
    // Only one line left, just clear it.
    TextLine& line = GetLine(ln);
    RemoveLineLength(line.Length());
    line.Clear(line_data);
    AddLineLength(line.Length());
    Notify(kLineUpdated, ChangeData(ln));
  } else {
    TextLines::iterator it(lines_.begin());
    std::advance(it, ln - 1);
    if (line_data != NULL) {
      *line_data = (*it)->data();
    }
    RemoveLineLength((*it)->Length());
    lines_.erase(it);
    Notify(kLineDeleted, ChangeData(ln));
  }
}

TextPoint TextBuffer::InsertText(const TextPoint& point, const std::wstring& text) {
  // Avoid to notify on every change, notify at last as few as possible.
  FreezeNotify();

  size_t line_count = 0;
  TextPoint insert_point = point;
  size_t p = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == LF) {
      insert_point = InsertString(insert_point, text.substr(p, i - p));
      insert_point = InsertChar(insert_point, LF);
      p = i + 1;
      ++line_count;
    }
  }

  if (p < text.size()) {
    insert_point = InsertString(insert_point, text.substr(p, text.size() - p));
  }

  ThawNotify();

  // Notify now.
  // Note (2013-04-20): Notify LineAdded first so that wrap infos can be updated
  // before LineUpdated.
  if (line_count > 0) {
    Notify(kLineAdded, ChangeData(point.y + 1, point.y + line_count));
  }
  Notify(kLineUpdated, ChangeData(point.y));

  return insert_point;
}

void TextBuffer::DeleteText(const TextRange& range, std::wstring* text) {
  if (range.first_point().y == range.last_point().y) {
    DeleteString(range.first_point(), range.last_point().x - range.first_point().x, text);
    return;
  }

  FreezeNotify();

  // First line.
  DeleteString(range.first_point(), std::wstring::npos, text);
  if (text != NULL) {
    *text += LF;
  }

  // Middle lines.
  for (Coord y = range.first_point().y + 1; y < range.last_point().y; ++y) {
    if (text != NULL) {
      std::wstring line_data;
      DeleteLine(range.first_point().y + 1, &line_data);
      *text += line_data + LF;
    } else {
      DeleteLine(range.first_point().y + 1);
    }
  }

  // Last line.
  if (range.last_point().x > 0) {
    if (text != NULL) {
      std::wstring str;
      DeleteString(TextPoint(0, range.first_point().y + 1), range.last_point().x, &str);
      *text += str;
    } else {
      DeleteString(TextPoint(0, range.first_point().y + 1), range.last_point().x);
    }
  }

  // Delete the line ending of first line.
  DeleteChar(TextPoint(LineLength(range.first_point().y), range.first_point().y));

  ThawNotify();

  // Avoid to notify on every change.
  // Note (2013-04-20): Notify LineDeleted first so that wrap infos can be updated
  // before LineUpdated.
  Notify(kLineDeleted, ChangeData(range.first_point().y + 1, range.last_point().y));
  Notify(kLineUpdated, ChangeData(range.first_point().y));
}

//------------------------------------------------------------------------------
// Listener operations.

void TextBuffer::AttachListener(BufferListener* listener) {
  listeners_.push_back(listener);
}

void TextBuffer::DetachListener(BufferListener* listener) {
  std::remove(listeners_.begin(), listeners_.end(), listener);
}

void TextBuffer::Notify(ChangeType type, const ChangeData& data) {
  if (notify_frozen_) {
    return;
  }

  foreach (BufferListener* listener, listeners_) {
    listener->OnBufferChange(type, data);
  }
}

void TextBuffer::FreezeNotify() {
  notify_frozen_ = true;
}

void TextBuffer::ThawNotify() {
  notify_frozen_ = false;
}

void TextBuffer::NotifyEncodingChange() {
  foreach (BufferListener* listener, listeners_) {
    listener->OnBufferEncodingChange();
  }
}

void TextBuffer::NotifyFileNameChange() {
  foreach (BufferListener* listener, listeners_) {
    listener->OnBufferFileNameChange();
  }
}

void TextBuffer::NotifySaved() {
  foreach (BufferListener* listener, listeners_) {
    listener->OnBufferSaved();
  }
}

//------------------------------------------------------------------------------
// Undo/redo

static bool MergeDeleteActions(DeleteAction* delete_action, EditAction* prev_edit_action) {
  if (prev_edit_action == NULL) {
    return false;
  }

  DeleteAction* prev_delete_action = prev_edit_action->AsDeleteAction();
  if (prev_delete_action == NULL) { // Not a delete action.
    return false;
  }

  if (prev_delete_action->text_unit() != delete_action->text_unit() ||
      prev_delete_action->seek_type() != delete_action->seek_type() ||
      prev_delete_action->ever_saved() != delete_action->ever_saved()) {
    // Different kind of delete actions.
    return false;
  }

  // Check time interval.
  wxTimeSpan max_interval = wxTimeSpan::Seconds(1);
  wxDateTime prev_timestamp = prev_delete_action->timestamp();
  prev_timestamp = prev_timestamp.Add(max_interval);
  if (prev_timestamp.IsEarlierThan(delete_action->timestamp())) {
    return false;
  }

  // Execute it before merge.
  delete_action->Exec();

  // Merge the delete actions.
  return prev_delete_action->Merge(delete_action);
}

EditAction* TextBuffer::AddEditAction(EditAction* edit_action) {
  MergeInsertCharActions();
  ClearEditActionList(redo_edit_actions_);

  DeleteAction* delete_action = edit_action->AsDeleteAction();
  if (delete_action != NULL && MergeDeleteActions(delete_action, GetUndoEditAction())) {
    delete edit_action;
    return GetUndoEditAction();
  } else {
    PushUndoEditAction(edit_action);
    // Execute the new action after it's added for modified state checking.
    edit_action->Exec();
    return edit_action;
  }
}

void TextBuffer::AddInsertCharAction(InsertCharAction* insert_char_action) {
  ClearEditActionList(redo_edit_actions_);

  if (insert_char_action->c() == LF) {
    MergeInsertCharActions();
    PushUndoEditAction(insert_char_action);
    insert_char_action->Exec();
  } else {
    if (!temp_insert_char_actions_.empty()) {
      InsertCharAction* prev_insert_char_action = temp_insert_char_actions_.back();

      TextPoint prev_point = prev_insert_char_action->point();
      TextPoint point = insert_char_action->point();

      if (prev_point.y != point.y || prev_point.x + 1 != point.x) {
        MergeInsertCharActions();
      }/* else if (!IsEditActionCoutinuousByTime(insert_char_action, prev_insert_char_action)) {
       CombineInsertCharActions();
       }*/
    }

    temp_insert_char_actions_.push_back(insert_char_action);
    insert_char_action->Exec();
  }
}

void TextBuffer::MergeInsertCharActions() {
  if (temp_insert_char_actions_.empty()) {
    return;
  }

  std::wstring str;
  str.resize(temp_insert_char_actions_.size());
  for (size_t i = 0; i < temp_insert_char_actions_.size(); ++i) {
    str[i] = temp_insert_char_actions_[i]->c();
  }

  size_t p = 0;
  while (true) {
    size_t i = IndexWordEnd(str, p, true);

    size_t sub_count = i - p;
    TextPoint point = temp_insert_char_actions_[p]->point();
    if (sub_count == 1) {
      PushUndoEditAction(new InsertCharAction(this, point, str[p]));
    } else { // > 1
      PushUndoEditAction(new InsertStringAction(this, point, str.substr(p, sub_count)));
    }

    if (i >= str.size()) {
      break;
    }

    p = i;
  }

  for (size_t i = 0; i < temp_insert_char_actions_.size(); ++i) {
    delete temp_insert_char_actions_[i];
  }
  temp_insert_char_actions_.clear();
}

EditAction* TextBuffer::Undo() {
  EditAction* edit_action = PopUndoEditAction();
  if (edit_action != NULL) {
    edit_action->Undo();
    PushRedoEditAction(edit_action);
  }
  return edit_action;
}

EditAction* TextBuffer::Redo() {
  if (!CanRedo()) {
    return NULL;
  }

  EditAction* edit_action = redo_edit_actions_.front();
  redo_edit_actions_.pop_front();
  PushUndoEditAction(edit_action);
  edit_action->Exec();
  return edit_action;
}

EditAction* TextBuffer::Repeat(const TextPoint& point) {
  if (!CanRepeat()) {
    return NULL;
  }

  // Pick up the last action and repeat it.

  MergeInsertCharActions();

  EditAction* new_edit_action = undo_edit_actions_.front()->Clone(point);
  if (new_edit_action == NULL) {
    return NULL;
  }
  new_edit_action->Exec();
  PushUndoEditAction(new_edit_action);
  return new_edit_action;
}

bool TextBuffer::CanUndo() const {
  return !undo_edit_actions_.empty() || !temp_insert_char_actions_.empty();
}

bool TextBuffer::CanRedo() const {
  return !redo_edit_actions_.empty();
}

bool TextBuffer::CanRepeat() const {
  if (CanRedo() || !CanUndo()) {
    return false;
  }

  // Check if the undo action is repeatable.

  if (!temp_insert_char_actions_.empty()) {
    return temp_insert_char_actions_.front()->CanRepeat();
  } else if (!undo_edit_actions_.empty()) {
    return undo_edit_actions_.front()->CanRepeat();
  }

  return false;
}

void TextBuffer::ClearEditActionList(std::list<EditAction*>& edit_actions) {
  foreach (EditAction* edit_action, edit_actions) {
    delete edit_action;
  }
  edit_actions.clear();
}

EditAction* TextBuffer::GetUndoEditAction() {
  MergeInsertCharActions();

  if (!undo_edit_actions_.empty()) {
    return undo_edit_actions_.front();
  }
  if (!saved_undo_edit_actions_.empty()) {
    return saved_undo_edit_actions_.front();
  }
  return NULL;
}

EditAction* TextBuffer::PopUndoEditAction() {
  MergeInsertCharActions();

  if (!undo_edit_actions_.empty()) {
    EditAction* edit_action = undo_edit_actions_.front();
    undo_edit_actions_.pop_front();
    return edit_action;
  }

  if (!saved_undo_edit_actions_.empty()) {
    EditAction* edit_action = saved_undo_edit_actions_.front();
    saved_undo_edit_actions_.pop_front();
    return edit_action;
  }

  return NULL;
}

void TextBuffer::PushUndoEditAction(EditAction* edit_action) {
  if (edit_action->ever_saved()) {
    assert(undo_edit_actions_.empty());
    assert(temp_insert_char_actions_.empty());
    saved_undo_edit_actions_.push_front(edit_action);
  } else {
    undo_edit_actions_.push_front(edit_action);
  }
}

void TextBuffer::PushRedoEditAction(EditAction* edit_action) {
  redo_edit_actions_.push_front(edit_action);
}

void TextBuffer::ClearEditActions() {
  ClearEditActionList(saved_undo_edit_actions_);
  ClearEditActionList(undo_edit_actions_);
  ClearEditActionList(redo_edit_actions_);
}

void TextBuffer::SaveUndoActions() {
  MergeInsertCharActions();

  reverse_foreach (EditAction* edit_action, undo_edit_actions_) {
    edit_action->set_ever_saved(true);
    saved_undo_edit_actions_.push_front(edit_action);
  }
  undo_edit_actions_.clear();

  last_saved_undo_count_ = saved_undo_edit_actions_.size();
}

bool TextBuffer::GetBracketPairOuterRange(const TextRange& range, TextRange& bracket_pair_range) const {
  TextPoint open_point;
  if (!FindOpenBracket(range.first_point(), open_point)) {
    return false;
  }

  wchar_t open_bracket = GetChar(open_point);

  TextPoint close_point = range.last_point();
  while (true) {
    if (!FindCloseBracket(close_point, close_point)) {
      return false;
    }
    if (IsBracketPair(open_bracket, GetChar(close_point))) {
      break;
    } else {
      ++close_point.x;
    }
  }

  ++close_point.x; // To the char last_point().
  bracket_pair_range.Set(open_point, close_point);
  return true;
}

bool TextBuffer::GetBracketPairInnerRange(const TextRange& range, TextRange& bracket_pair_range) const {
  TextRange outer_range;
  if (!GetBracketPairOuterRange(range, outer_range)) {
    return false;
  }

  TextPoint first_point = outer_range.first_point();
  TextPoint last_point = outer_range.last_point();

  ++first_point.x;
  if (last_point.x > 0) {
    --last_point.x;
  } else {
    --last_point.y;
    last_point.x = LineLength(last_point.y);
  }

  bracket_pair_range.Set(first_point, last_point);
  return true;
}

bool TextBuffer::IsBracketPairInnerRange(TextRange& bracket_pair_range) const {
  TextPoint first_point = bracket_pair_range.first_point();
  TextPoint last_point = bracket_pair_range.last_point();

  if (first_point.x > 0) {
    --first_point.x;
  } else {
    if (first_point.y > 1) {
      --first_point.y;
      first_point.x = LineLength(first_point.y);
    } else {
      return false;
    }
  }
  if (!IsOpenBracket(GetChar(first_point))) {
    return false;
  }

  // Don't have to increase point last_point().
  return IsBracketPair(GetChar(first_point), GetChar(last_point));
}

bool TextBuffer::IncreaseRange(TextRange& range) const {
  TextPoint first_point = range.first_point();
  TextPoint last_point = range.last_point();

  if (first_point.x > 0) {
    --first_point.x;
  } else {
    if (first_point.y > 1) {
      --first_point.y;
      first_point.x = LineLength(first_point.y);
    } else {
      return false;
    }
  }

  const Coord end_line_length = LineLength(last_point.y);
  if (last_point.x < end_line_length) {
    ++last_point.x;
  } else {
    if (last_point.y < LineCount()) {
      ++last_point.y;
    } else {
      return false;
    }
  }

  range.Set(first_point, last_point);
  return true;
}

static
bool CheckBracket(wchar_t c,
                  Bracket (*check_bracket1)(wchar_t),
                  Bracket (*check_bracket2)(wchar_t),
                  int counter[kBracketCount]) {
  Bracket bracket = check_bracket1(c);
  if (bracket != kNoBracket) {
    if (counter[bracket] == 0) {
      return true;
    } else {
      --counter[bracket];
    }
  } else if ((bracket = check_bracket2(c)) != kNoBracket) {
    ++counter[bracket];
  }
  return false;
}

bool TextBuffer::FindOpenBracket(const TextPoint& point, TextPoint& found_point) const {
  int counter[kBracketCount] = { 0 };

  const std::wstring& line_data = GetLine(point.y).data();
  Coord x = point.x - 1;
  for (; x >= 0; --x) {
    if (CheckBracket(line_data[x], CheckOpenBracket, CheckCloseBracket,
                     counter)) {
      found_point.Set(x, point.y);
      return true;
    }
  }

  for (Coord y = point.y - 1; y > 0; --y) {
    const std::wstring& line_data = GetLine(y).data();
    Coord x = LineLength(y) - 1;
    for (; x >= 0; --x) {
      if (CheckBracket(line_data[x], CheckOpenBracket, CheckCloseBracket,
                       counter)) {
        found_point.Set(x, y);
        return true;
      }
    }
  }

  return false;
}

bool TextBuffer::FindCloseBracket(const TextPoint& point, TextPoint& found_point) const {
  int counter[kBracketCount] = { 0 };

  const std::wstring& line_data = GetLine(point.y).data();
  const Coord line_length = LineLength(point.y);
  Coord x = point.x;
  for (; x < line_length; ++x) {
    if (CheckBracket(line_data[x], CheckCloseBracket, CheckOpenBracket,
                     counter)) {
      found_point.Set(x, point.y);
      return true;
    }
  }

  const Coord line_count = LineCount();
  for (Coord y = point.y + 1; y <= line_count; ++y) {
    const std::wstring& line_data = GetLine(y).data();
    const Coord line_length = LineLength(y);
    Coord x = 0;
    for (; x < line_length; ++x) {
      if (CheckBracket(line_data[x], CheckCloseBracket, CheckOpenBracket,
                       counter)) {
        found_point.Set(x, y);
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------

void TextBuffer::InsertLine(Coord ln, TextLine* line) {
  // FIXME
  assert(lines_.size() < kUCoordMax);
  lines_.insert(lines_.begin() + ln, line);
  AddLineLength(line->Length());
}

void TextBuffer::AppendLine(const std::wstring& line_data) {
  // FIXME
  assert(lines_.size() < kUCoordMax);
  TextLine* line = new TextLine(NewLineId(), line_data);
  lines_.push_back(line);
  AddLineLength(line->Length());
}

size_t TextBuffer::NewLineId() {
  return ++line_id_;
}

#ifdef __WXDEBUG__
void TextBuffer::LogLineLengthTable() {
  for (size_t i = 0; i < line_length_table_.size(); ++i) {
    wxLogDebug(wxT("%d: %d"), i, line_length_table_[i]);
  }
}
#endif // __WXDEBUG__

Coord TextBuffer::GetMaxLineLength() const {
  if (line_length_table_.empty()) {
    return 0;
  }
  size_t i = line_length_table_.size() - 1;
  while (i > 0 && line_length_table_[i] == 0) {
    --i;
  }
  return i;
}

void TextBuffer::AddLineLength(size_t line_length) {
  // Extend the table.
  if (line_length >= line_length_table_.size()) {
    line_length_table_.resize(line_length + 1, 0);
  }
  ++line_length_table_[line_length];

  //ShrinkLineLengthTable();
}

void TextBuffer::RemoveLineLength(size_t line_length) {
  assert(line_length < line_length_table_.size());
  if (line_length_table_[line_length] > 0) {
    --line_length_table_[line_length];
  }
}

void TextBuffer::ShrinkLineLengthTable() {
  // TODO
  //size_t max_line_length = GetMaxLineLength();
  //if (max_line_length + 1 < line_length_table_.size()) {
  //  line_length_table_.resize(max_line_length);
  //}
}

} } // namespace jil::editor
